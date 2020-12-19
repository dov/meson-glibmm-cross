//======================================================================
//  mesh-viewer.h - A viewer for a mesh
//
//  Dov Grobgeld <dov.grobgeld@gmail.com>
//  Wed Dec 16 23:14:11 2020
//----------------------------------------------------------------------
#ifndef MESH_VIEWER_H
#define MESH_VIEWER_H

#include <gtkmm.h>
#include <epoxy/gl.h>

class MeshViewer : public Gtk::GLArea
{
  public:
  MeshViewer();

  void on_realize() override;
  void on_unrealize() override;
  bool on_render (const Glib::RefPtr< Gdk::GLContext >& context) override;
  bool on_button_press_event (GdkEventButton* button_event) override;
  bool on_motion_notify_event (GdkEventMotion* motion_event) override;

  private:
  guint m_vao {0};
  guint m_buffer {0};
  guint m_program {0};
  guint m_mvp_loc {0};
  guint m_position_index {0};
  guint m_color_index {0};  

  // used by the trackball
  float m_view_quat[4] = { 0.0, 0.0, 0.0, 1.0 };
  
  int m_begin_x;
  int m_begin_y;

  void draw_mesh();
};

#endif /* MESH-VIEWER */
