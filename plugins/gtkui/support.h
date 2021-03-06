#ifndef __GTKUI_SUPPORT_H
#define __GTKUI_SUPPORT_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#if GTK_CHECK_VERSION(3,0,0)
#include <gdk/gdkkeysyms-compat.h>
#else
#include <gdk/gdkkeysyms.h>
#endif

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  define Q_(String) g_strip_context ((String), gettext (String))
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#ifndef _
#  define _(String) (String)
#endif
#  define Q_(String) g_strip_context ((String), (String))
#  define N_(String) (String)
#endif


/*
 * Public Functions.
 */

/*
 * This function returns a widget in a component created by Glade.
 * Call it with the toplevel widget in the component (i.e. a window/dialog),
 * or alternatively any widget in the component, and the name of the widget
 * you want returned.
 */
GtkWidget*  lookup_widget              (GtkWidget       *widget,
                                        const gchar     *widget_name);


/* Use this function to set the directory containing installed pixmaps. */
void        add_pixmap_directory       (const gchar     *directory);


/*
 * Private Functions.
 */

/* This is used to create the pixmaps used in the interface. */
GtkWidget*  create_pixmap              (GtkWidget       *widget,
                                        const gchar     *filename);

/* This is used to create the pixbufs used in the interface. */
GdkPixbuf*  create_pixbuf              (const gchar     *filename);

/* This is used to set ATK action descriptions. */
void        glade_set_atk_action_description (AtkAction       *action,
                                              const gchar     *action_name,
                                              const gchar     *description);

#if GTK_CHECK_VERSION(3,0,0)
GtkWidget *
gtk_combo_box_entry_new_text(void);

void
gtk_dialog_set_has_separator (GtkDialog *dlg, gboolean has);
#endif

#if !GTK_CHECK_VERSION(2,22,0)
GdkDragAction
gdk_drag_context_get_selected_action (GdkDragContext *context);
GList *
gdk_drag_context_list_targets (GdkDragContext *context);
#endif

#if !GTK_CHECK_VERSION(2,24,0)
#define GTK_COMBO_BOX_TEXT GTK_COMBO_BOX
typedef GtkComboBox GtkComboBoxText;
GtkWidget *gtk_combo_box_text_new ();
GtkWidget *gtk_combo_box_text_new_with_entry   (void);
void gtk_combo_box_text_append_text (GtkComboBoxText *combo_box, const gchar *text);
void gtk_combo_box_text_insert_text (GtkComboBoxText *combo_box, gint position, const gchar *text);
void gtk_combo_box_text_prepend_text (GtkComboBoxText *combo_box, const gchar *text);
gchar *gtk_combo_box_text_get_active_text  (GtkComboBoxText *combo_box);
#endif

#endif
