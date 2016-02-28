#include <glib.h>
#include <glib/gi18n.h>

#include <version.h>
#include <claws.h>
#include <plugin.h>
#include <hooks.h>
#include <utils.h>
#include <log.h>
#include <mainwindow.h>
#include <main.h>
#include <viewtypes.h>
#include <folderview.h>

#include <libappindicator/app-indicator.h>

#include <gtk/gtkmenu.h>

#define P_UNUSED(v) ((void)v)
#define BAD_HOOK ((guint)-1)

#define PLUGIN_VERSION ("0.0.1")
#define PLUGIN_NAME ("appindicator Notifier")
#define PLUGIN_DESCRIPTION ("This plugin provides an appindicator system tray icon for use with KDE5.")

#define PLUGIN_NOMAILS_ICON "claws-mail"
#define PLUGIN_NEWMAILS_ICON "mail-unread-new"

typedef struct {
  guint unread;
  guint total;
} MessageCounts;

static MessageCounts msg_count;
static GHashTable *msg_count_hash;
static AppIndicator *indicator;
static guint hook_mw_closed;
static guint hook_folder_update;
static guint hook_item_update;

static inline void toggle_cb(void);
static inline void quit_cb(void);

static inline void show_main_window(MainWindow *mw)
{
	static gboolean fixed = FALSE;

	GtkWindow *mww = GTK_WINDOW(mw->window);
	gtk_window_deiconify(mww);
	gtk_window_set_skip_taskbar_hint(mww, FALSE);
	main_window_show(mw);
	gtk_window_present(mww);

	if (!fixed) {
		gtk_widget_queue_resize(mw->folderview->ctree);
		fixed = TRUE;
	}
}

static inline void toggle_main_window(void)
{
	MainWindow *mw = mainwindow_get_mainwindow();
	GtkWidget *mww = 0;
	if (!mw) return;

	mww = GTK_WIDGET(mw->window);
	if (gtk_widget_get_visible(mww)) {
		if((gdk_window_get_state(mww->window) & GDK_WINDOW_STATE_ICONIFIED) || mainwindow_is_obscured()) {
			show_main_window(mw);
		} else {
			main_window_hide(mw);
		}
	} else {
		show_main_window(mw);
	}
}

static inline GtkWidget *separator(GtkWidget *menu)
{
	GtkWidget *item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	return item;
}

static inline GtkWidget *menu_item(GtkWidget *menu, const char *label, const char *icon, GCallback cb, gpointer data)
{
	GtkWidget *item;
	GtkWidget *pic;

	if (icon)
		item = gtk_image_menu_item_new_with_mnemonic(label);
	else
		item = gtk_menu_item_new_with_mnemonic(label);

	if (menu)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	if (cb)
		g_signal_connect(G_OBJECT(item), "activate", cb, data);

	if (icon) {
		pic = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), pic);
	}

	gtk_widget_show_all(item);
	return item;
}

static inline GtkWidget *generate_menu(void)
{
	static GtkWidget *menu = 0;
	GtkWidget *toggle_item = 0;

	if (menu)
		gtk_widget_destroy(menu);

	menu = gtk_menu_new();

	toggle_item = menu_item(menu, _("_Toggle"), "window-suppressed", G_CALLBACK(&toggle_cb), 0);
	menu_item(menu, _("_Get mail"), "mail-receive", 0, 0);
	separator(menu);
	menu_item(menu, _("_Quit"), GTK_STOCK_QUIT, G_CALLBACK(&quit_cb), 0);

	gtk_widget_show_all(menu);
	app_indicator_set_secondary_activate_target(indicator, toggle_item);
	return menu;
}

static inline void toggle_cb(void)
{
	toggle_main_window();
}

static inline void quit_cb(void)
{
	MainWindow *mw = mainwindow_get_mainwindow();

	if (mw->lock_count == 0) {
		app_will_exit(0, mw);
	}
}

static inline void msg_count_update_cb(gpointer key, gpointer value, gpointer data)
{
	P_UNUSED(key);
	P_UNUSED(data);

	MessageCounts *ic = (MessageCounts *)value;
	msg_count.unread += ic->unread;
	msg_count.total  += ic->total;
}

static inline void update_counts_cb(FolderItem *item, gpointer data)
{
	gchar *id;
	MessageCounts *count;
	GHashTable *hash = (GHashTable *)data;

	switch (item->folder->klass->type) {
		case F_MH:
		case F_MBOX:
		case F_MAILDIR:
		case F_IMAP:
		case F_NEWS: {
			break;
		}
		default: {
			if (!strcmp(item->folder->klass->uistr, "vCalendar"))
				break;
			if (!strcmp(item->folder->klass->uistr, "RSSyl"))
				break;
			return;
		}
	}

	id = folder_item_get_identifier(item);
	if (!id)
		return;

	count = (MessageCounts *)g_hash_table_lookup(hash, id);
	if (!count) {
		count = (MessageCounts *)g_new0(MessageCounts, 1);
		g_hash_table_insert(hash, id, count);
	} else {
		g_free(id);
	}

	count->unread = item->unread_msgs;
	count->total  = item->total_msgs;
}

static inline void update_counts(FolderItem *removed)
{
	if (!msg_count_hash)
		msg_count_hash = g_hash_table_new_full(&g_str_hash, &g_str_equal, &g_free, &g_free);

	folder_func_to_all_folders(&update_counts_cb, msg_count_hash);
	if (removed) {
		gchar *id = folder_item_get_identifier(removed);
		if (id) {
			g_hash_table_remove(msg_count_hash, id);
			g_free(id);
		}
	}

	memset(&msg_count, 0, sizeof(msg_count));
	g_hash_table_foreach(msg_count_hash, &msg_count_update_cb, 0);

	if (indicator) {
		gchar buff[1024] = {};
		gchar gbuff[1024] = {};
		g_snprintf(buff, sizeof(buff), _("%u unread message(s)"), msg_count.unread);
		g_snprintf(buff, sizeof(buff), _("%u unread message(s)"), (guint)-1);
		if (msg_count.unread) {
			app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ATTENTION);
			app_indicator_set_icon(indicator, PLUGIN_NEWMAILS_ICON);
		} else {
			app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
			app_indicator_set_icon(indicator, PLUGIN_NOMAILS_ICON);
		}
		app_indicator_set_label(indicator, buff, gbuff);
	}
}

static inline gboolean main_window_closed_hook(gpointer source, gpointer hookdata)
{
	P_UNUSED(hookdata);

	if (source) {
		gboolean *close_allowed = (gboolean *)source;
		MainWindow *mw = mainwindow_get_mainwindow();

		if (mw) {
			*close_allowed = FALSE;
			if (gtk_widget_get_visible(GTK_WIDGET(mw->window)))
				main_window_hide(mw);
		}
	}

	return FALSE;
}

static inline gboolean folder_update_hook(gpointer source, gpointer hookdata)
{
	P_UNUSED(hookdata);

	FolderUpdateData *data = (FolderUpdateData *)source;
	FolderItem *removed = 0;

	if (data->update_flags & FOLDER_REMOVE_FOLDERITEM)
		removed = data->item;

	update_counts(removed);

	return FALSE;
}

static inline gboolean item_update_hook(gpointer source, gpointer hookdata)
{
	P_UNUSED(hookdata);

	FolderItemUpdateData *data = (FolderItemUpdateData *)source;
	if (folder_has_parent_of_type(data->item, F_DRAFT))
		return FALSE;

	update_counts(0);

	return FALSE;
}

gint plugin_init(gchar **error)
{
	hook_mw_closed = hooks_register_hook(MAIN_WINDOW_CLOSE, &main_window_closed_hook, 0);
	if (hook_mw_closed == BAD_HOOK) {
		*error = g_strdup(_("Failed to register main window close hook"));
		return -1;
	}

	hook_folder_update = hooks_register_hook(FOLDER_UPDATE_HOOKLIST, &folder_update_hook, 0);
	if (hook_folder_update == BAD_HOOK) {
		*error = g_strdup(_("Failed to register folder update hook"));
		hooks_unregister_hook(MAIN_WINDOW_CLOSE, hook_mw_closed);
		return -1;
	}

	hook_item_update = hooks_register_hook(FOLDER_ITEM_UPDATE_HOOKLIST, &item_update_hook, 0);
	if (hook_item_update == BAD_HOOK) {
		*error = g_strdup(_("Failed to register item update hook"));
		hooks_unregister_hook(MAIN_WINDOW_CLOSE, hook_mw_closed);
		hooks_unregister_hook(FOLDER_UPDATE_HOOKLIST, hook_folder_update);
		return -1;
	}

	indicator = app_indicator_new("claws-mail-indicator", PLUGIN_NOMAILS_ICON, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
	app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
	app_indicator_set_icon(indicator, PLUGIN_NOMAILS_ICON);
	app_indicator_set_attention_icon(indicator, PLUGIN_NEWMAILS_ICON);
	app_indicator_set_title(indicator, "Claws Mail");
	app_indicator_set_menu(indicator, GTK_MENU(generate_menu()));

	update_counts(0);

	return 0;
}

gboolean plugin_done(void)
{
	if (indicator) {
		app_indicator_set_status(indicator, APP_INDICATOR_STATUS_PASSIVE);
		g_object_unref(indicator);
		indicator = 0;
	}

	hooks_unregister_hook(MAIN_WINDOW_CLOSE, hook_mw_closed);
	hooks_unregister_hook(FOLDER_UPDATE_HOOKLIST, hook_folder_update);
	hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_item_update);

	if (msg_count_hash) {
		g_hash_table_destroy(msg_count_hash);
		msg_count_hash = 0;
	}

	return TRUE;
}

const gchar *plugin_name(void)
{
	return _(PLUGIN_NAME);
}

const gchar *plugin_desc(void)
{
	return _(PLUGIN_DESCRIPTION);
}

const gchar *plugin_type(void)
{
	return "Common";
}

const gchar *plugin_licence(void)
{
	return "GPL3+";
}

const gchar *plugin_version(void)
{
	return PLUGIN_VERSION;
}

struct PluginFeature *plugin_provides(void)
{
	static struct PluginFeature features[] = {
		{PLUGIN_OTHER,   N_(PLUGIN_NAME)},
		{PLUGIN_NOTHING, NULL}
	};
	return features;
}
