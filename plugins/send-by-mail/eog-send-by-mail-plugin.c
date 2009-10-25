/* SendByMail  -- Send images per email
 *
 * Copyright (C) 2009 The Free Software Foundation
 *
 * Author: Felix Riemann  <friemann@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include <eog/eog-image.h>
#include <eog/eog-thumb-view.h>
#include <eog/eog-window.h>

#include "eog-send-by-mail-plugin.h"


#define WINDOW_DATA_KEY "EogSendByMailWindowData"

EOG_PLUGIN_REGISTER_TYPE(EogSendByMailPlugin, eog_send_by_mail_plugin)

typedef struct
{
	GtkActionGroup *ui_action_group;
	guint           ui_menuitem_id;
} WindowData;

static void free_window_data (WindowData *data);
static void send_by_mail_cb (GtkAction *action, EogWindow *window);

static const gchar * const ui_definition =
	"<ui><menubar name=\"MainMenu\">"
	"<menu name=\"ToolsMenu\" action=\"Tools\">"
	"<menuitem action=\"EogPluginSendByMail\"/>"
	"</menu></menubar>"
	"<popup name=\"ViewPopup\"><separator/>"
        "<menuitem action=\"EogPluginSendByMail\"/><separator/>"
        "</popup></ui>";

static const GtkActionEntry action_entries[] =
{
	{ "EogPluginSendByMail",
	  "mail-message-new",
	  N_("Send by Mail"),
	  NULL,
	  N_("Send the selected images by mail"),
	  G_CALLBACK (send_by_mail_cb) }
};

static void
eog_send_by_mail_plugin_init (EogSendByMailPlugin *plugin)
{
}

static void
impl_activate (EogPlugin *plugin,
	       EogWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;

	manager = eog_window_get_ui_manager (window);
	data = g_slice_new (WindowData);

	data->ui_action_group = gtk_action_group_new ("EogSendByMailPluginActions");

	gtk_action_group_set_translation_domain (data->ui_action_group,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (data->ui_action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      window);

	gtk_ui_manager_insert_action_group (manager,
					    data->ui_action_group,
					    -1);

	data->ui_menuitem_id = gtk_ui_manager_add_ui_from_string (manager,
								  ui_definition,
								  -1, NULL);

	g_object_set_data_full (G_OBJECT (window),
				WINDOW_DATA_KEY,
				data,
				(GDestroyNotify) free_window_data);
}

static void
impl_deactivate	(EogPlugin *plugin,
		 EogWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;

	manager = eog_window_get_ui_manager (window);

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	gtk_ui_manager_remove_ui (manager,
				  data->ui_menuitem_id);

	gtk_ui_manager_remove_action_group (manager,
					    data->ui_action_group);

	g_object_set_data (G_OBJECT (window),
			   WINDOW_DATA_KEY,
			   NULL);
}

static void
eog_send_by_mail_plugin_class_init (EogSendByMailPluginClass *klass)
{
	EogPluginClass *plugin_class = EOG_PLUGIN_CLASS (klass);

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
}

static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);

	g_object_unref (data->ui_action_group);

	g_slice_free (WindowData, data);
}

static void
send_by_mail_cb (GtkAction *action, EogWindow *window)
{
	GdkScreen *screen = NULL;
	GtkWidget *tview = NULL;
	GList *images = NULL, *image = NULL;
	GString *uri = NULL;
	gboolean first = TRUE;

	g_return_if_fail (EOG_IS_WINDOW (window));

	if (gtk_widget_has_screen (GTK_WIDGET (window)))
		screen = gtk_widget_get_screen (GTK_WIDGET (window));

	tview = eog_window_get_thumb_view (window);
	images = eog_thumb_view_get_selected_images (EOG_THUMB_VIEW (tview));
	uri = g_string_new ("mailto:?attach=");

	for (image = images; image != NULL; image = image->next) {
		EogImage *img = EOG_IMAGE (image->data);
		GFile *file;
		gchar *path;

		file = eog_image_get_file (img);
		if (!file) {
			g_object_unref (img);
			continue;
		}

		path = g_file_get_path (file);
		if (first) {
			uri = g_string_append (uri, path);
			first = FALSE;
		} else {
			g_string_append_printf (uri, "&attach=%s", path);
		}
		g_free (path);
		g_object_unref (file);
		g_object_unref (img);
	}
	g_list_free (images);

	/* TODO: Error handling/reporting? */
	gtk_show_uri (screen, uri->str, gtk_get_current_event_time (), NULL);

	g_string_free (uri, TRUE);
}
