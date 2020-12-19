// A basic viewer that shows a fixed triangular mesh.

#include "mesh-viewer.h"
#include <iostream>
#include <fmt/core.h>
#include "trackball.h"

using namespace std;
using namespace fmt;

#define VIEW_INIT_AXIS_X 1.0
#define VIEW_INIT_AXIS_Y 0.0
#define VIEW_INIT_AXIS_Z 0.0
#define VIEW_INIT_ANGLE  20.0
#define DIG_2_RAD (G_PI / 180.0)
#define RAD_2_DIG (180.0 / G_PI)

// Constructor
MeshViewer::MeshViewer()
{
  this->add_events (
    Gdk::EventMask(GDK_BUTTON1_MOTION_MASK    |
                   GDK_BUTTON_PRESS_MASK      |
                   GDK_VISIBILITY_NOTIFY_MASK
                   ));

}

// Example data from ebassi glarea_example

/* position and color information for each vertex */
struct vertex_info {
  float position[3];
  float color[3];
};

/* the vertex data is constant */
static const struct vertex_info vertex_data[] = {
  { {  0.0f,  0.500f, 0.0f }, { 1.f, 0.f, 0.f } },
  { {  0.5f, -0.366f, 0.0f }, { 0.f, 1.f, 0.f } },
  { { -0.5f, -0.366f, 0.0f }, { 0.f, 0.f, 1.f } },
};

static void
init_buffers (guint  position_index,
              guint  color_index,
              guint *vao_out)
{
  guint vao, buffer;

  /* we need to create a VAO to store the other buffers */
  glGenVertexArrays (1, &vao);
  glBindVertexArray (vao);

  /* this is the VBO that holds the vertex data */
  glGenBuffers (1, &buffer);
  glBindBuffer (GL_ARRAY_BUFFER, buffer);
  glBufferData (GL_ARRAY_BUFFER, sizeof (vertex_data), vertex_data, GL_STATIC_DRAW);

  /* enable and set the position attribute */
  glEnableVertexAttribArray (position_index);
  glVertexAttribPointer (position_index, 3, GL_FLOAT, GL_FALSE,
                         sizeof (struct vertex_info),
                         (GLvoid *) (G_STRUCT_OFFSET (struct vertex_info, position)));

  /* enable and set the color attribute */
  glEnableVertexAttribArray (color_index);
  glVertexAttribPointer (color_index, 3, GL_FLOAT, GL_FALSE,
                         sizeof (struct vertex_info),
                         (GLvoid *) (G_STRUCT_OFFSET (struct vertex_info, color)));

  /* reset the state; we will re-enable the VAO when needed */
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glBindVertexArray (0);

  /* the VBO is referenced by the VAO */
  glDeleteBuffers (1, &buffer);

  if (vao_out != NULL)
    *vao_out = vao;
}


// Returns the shader
static guint
create_shader (int shader_type,
               const Glib::RefPtr<const Glib::Bytes>& source)
{
  guint shader = glCreateShader (shader_type);
  
  // Get pointer and size
  gsize len=0;
  const char* src = (const char*)source->get_data(len);

  // Turn into GL structure
  int lengths[] = { (int)len };
  glShaderSource (shader, 1, &src, lengths);
  glCompileShader (shader);

  int status;
  glGetShaderiv (shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE)
    {
      int log_len;
      glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &log_len);

      string error_string;
      error_string.resize(log_len+1);
      glGetShaderInfoLog (shader, log_len, NULL, &error_string[0]);

      glDeleteShader (shader);
      shader = 0;

      throw runtime_error(
        format(
          "Compilation failure in {} shader: {}",
          shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
          error_string));
    }

  return shader;
}

void MeshViewer::on_realize()
{
  printf("On realize...\n");
  Gtk::GLArea::on_realize();

  make_current();

  // TBD - init the shaders
  /* load the vertex shader */
  Glib::RefPtr<const Glib::Bytes> vertex_source = Gio::Resource::lookup_data_global("/shaders/vertex.glsl");
  Glib::RefPtr<const Glib::Bytes> fragment_source = Gio::Resource::lookup_data_global("/shaders/fragment.glsl");
  
  guint vertex = create_shader(GL_VERTEX_SHADER, vertex_source);
  guint fragment = create_shader(GL_FRAGMENT_SHADER, fragment_source);
  m_program = glCreateProgram ();
  glAttachShader (m_program, vertex);
  glAttachShader (m_program, fragment);
  glLinkProgram (m_program);

  int status = 0;
  glGetProgramiv (m_program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE)
    {
      int log_len = 0;
      glGetProgramiv (m_program, GL_INFO_LOG_LENGTH, &log_len);

      string error_string;
      error_string.resize(log_len+1);
      glGetProgramInfoLog (m_program, log_len, NULL, &error_string[0]);

      glDeleteProgram (m_program);
      throw runtime_error(
        format(
          "Linking failure in program: {}",
          error_string));
    }

  /* get the location of the "mvp" uniform */
  m_mvp_loc = glGetUniformLocation (m_program, "mvp");

  /* get the location of the "position" and "color" attributes */
  m_position_index = glGetAttribLocation (m_program, "position");
  m_color_index = glGetAttribLocation (m_program, "color");

  /* the individual shaders can be detached and destroyed */
  glDetachShader (m_program, vertex);
  glDetachShader (m_program, fragment);
  glDeleteShader (vertex);
  glDeleteShader (fragment);

  // Init the buffer
  init_buffers (m_position_index, m_color_index, &m_vao);

  // Set up the initial view transformation
  float sine = sin (0.5 * VIEW_INIT_ANGLE * DIG_2_RAD);
  m_view_quat[0] = VIEW_INIT_AXIS_X * sine;
  m_view_quat[1] = VIEW_INIT_AXIS_Y * sine;
  m_view_quat[2] = VIEW_INIT_AXIS_Z * sine;
  m_view_quat[3] = cos (0.5 * VIEW_INIT_ANGLE * DIG_2_RAD);
}

void MeshViewer::on_unrealize()
{
  Gtk::GLArea::on_unrealize();
}

bool MeshViewer::on_render (const Glib::RefPtr< Gdk::GLContext >& context)
{
  printf("on_render\n");

  try
  {
    throw_if_error();
    glClearColor(0.6, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
  
    draw_mesh();

    glFlush();
  }
  catch(const Gdk::GLError& gle)
  {
    cerr << "An error occurred in the render callback of the GLArea" << endl;
    cerr << gle.domain() << "-" << gle.code() << "-" << gle.what() << endl;
    return false;
  }
  

  return true;
  
}

// Draw the mesh (or meshes)
void MeshViewer::draw_mesh()
{
  cout << "draw_mesh\n";
  float mvp[4][4];

  build_rotmatrix(mvp, m_view_quat);

  glUseProgram(m_program);

  /* update the "mvp" matrix we use in the shader */
  glUniformMatrix4fv (m_mvp_loc, 1, GL_FALSE, &mvp[0][0]);

  /* use the buffers in the VAO */
  glBindVertexArray (m_vao);

  /* draw the three vertices as a triangle */
  glDrawArrays (GL_TRIANGLES, 0, 3);

  glBindVertexArray (0);
  glUseProgram(0);
}

bool MeshViewer::on_button_press_event (GdkEventButton* button_event)
{
  if (button_event->button == 1) {
    m_begin_x = button_event->x;
    m_begin_y = button_event->y;
    return true;
  }
    
  return false;
}

bool MeshViewer::on_motion_notify_event (GdkEventMotion* motion_event)
{
  double w = get_window()->get_width();
  double h = get_window()->get_height();
  double x = motion_event->x;
  double y = motion_event->y;
  float d_quat[4];

  if (motion_event->state & GDK_BUTTON1_MASK)
  {
    trackball (d_quat,
               (2.0 * m_begin_x - w) / w,
               (h - 2.0 * m_begin_y) / h,
               (2.0 * x - w) / w,
               (h - 2.0 * y) / h);
    add_quats (d_quat, m_view_quat, m_view_quat);
  }
    
  m_begin_x = x;
  m_begin_y = y;

  Gdk::Rectangle rect;
  rect.set_x(0);
  rect.set_y(0);
  rect.set_width(w);
  rect.set_height(h);

  get_window()->invalidate_rect(rect, true);

  return true;
}
