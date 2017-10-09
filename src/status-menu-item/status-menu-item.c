#include <hildon/hildon-button.h>
#include <hildon/hildon-banner.h>
#include <libhildondesktop/libhildondesktop.h>
#include <libconnui.h>
#include <libintl.h>
#include <icd/dbus_api.h>
#include <osso-log.h>
#include <string.h>

#include "config.h"

#define _(x) dgettext(GETTEXT_PACKAGE, x)

#define CONNUI_CELLULAR_STATUS_MENU_ITEM_TYPE (connui_cellular_status_menu_item_get_type())
#define CONNUI_CELLULAR_STATUS_MENU_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONNUI_CELLULAR_STATUS_MENU_ITEM_TYPE, ConnuiCellularStatusMenuItem))

typedef struct _ConnuiCellularStatusMenuItem ConnuiCellularStatusMenuItem;
typedef struct _ConnuiCellularStatusMenuItemClass ConnuiCellularStatusMenuItemClass;
typedef struct _ConnuiCellularStatusMenuItemPrivate ConnuiCellularStatusMenuItemPrivate;

struct _ConnuiCellularStatusMenuItem
{
  HDStatusMenuItem parent;
  ConnuiCellularStatusMenuItemPrivate *priv;
};

struct _ConnuiCellularStatusMenuItemClass
{
  HDStatusMenuItemClass parent;
};

struct _ConnuiCellularStatusMenuItemPrivate
{
  struct network_state state;
  guint status;
  gboolean offline;
  osso_context_t *osso_context;
  struct pixbuf_cache *pixbuf_cache;
  osso_display_state_t display_state;
  int needs_display_state_update;
};

/* FIXME - get all enum members and move it to libconnui.h */
enum inetstate_status
{
  FLIGHTMODE = 0
};

gchar *current_mode_icon;
gchar *current_bars_icon;

HD_DEFINE_PLUGIN_MODULE(ConnuiCellularStatusMenuItem,
                        connui_cellular_status_menu_item,
                        HD_TYPE_STATUS_MENU_ITEM)

static void
connui_cellular_status_menu_item_class_finalize(ConnuiCellularStatusMenuItemClass *klass)
{
}

static void
connui_cellular_status_menu_item_update_icon(ConnuiCellularStatusMenuItem *item)
{
  GdkPixbuf *pixbuf;
  GdkPixbuf *bars_icon_pixbuf;
  GdkPixbuf *bars_icon_pixbuf2;
  GdkPixbuf *mode_icon_pixbuf;
  gchar *mode_icon;
  gchar *bars_icon;
  ConnuiCellularStatusMenuItemPrivate *priv = item->priv;
  if (priv->display_state == OSSO_DISPLAY_OFF)
  {
    priv->needs_display_state_update = TRUE;
    return;
  }
  guint status = priv->status;
  if (status != 3 && status != 0 && status != 5)
  {
    if (priv->state.network_reg_status > NETWORK_REG_STATUS_ROAM_BLINK)
    {
      if (priv->state.network_reg_status == NETWORK_REG_STATUS_NOSERV_NOSIM)
      {
        mode_icon = 0;
        bars_icon = "statusarea_cell_off";
      }
      mode_icon = 0;
      bars_icon = "statusarea_cell_level0";
    }
    else
    {
      char rat = priv->state.rat;
      if (rat == 2)
      {
        if (priv->state.network_radio_state == 1)
          mode_icon = "statusarea_cell_mode_3_5g";
        else
          mode_icon = "statusarea_cell_mode_3g";
      }
      else if (rat == 1)
      {
        if (priv->state.supported_services & 4)
          mode_icon = "statusarea_cell_mode_2_5g";
        else
          mode_icon = "statusarea_cell_mode_2g";
      }
      else
      {
        g_log(v4, G_LOG_LEVEL_WARNING, "status->rat unknown %hhu!", rat);
        mode_icon = 0;
      }
      char bars = priv->state.network_signals_bar;
      if (bars > 80)
      {
        bars_icon = "statusarea_cell_level5";
      }
      else if (bars > 60)
      {
        bars_icon = "statusarea_cell_level4";
      }
      else if (bars > 40)
      {
        bars_icon = "statusarea_cell_level3";
      }
      else if (bars > 20)
      {
        bars_icon = "statusarea_cell_level2";
      }
      else if (priv->state.network_signals_bar)
      {
        bars_icon = "statusarea_cell_level1";
      }
      else
      {
        bars_icon = "statusarea_cell_level0";
      }
    }
  }
  else
  {
    mode_icon = 0;
    bars_icon = "statusarea_cell_off";
  }
  if (priv->offline)
  {
    mode_icon = "statusarea_offline_mode";
    bars_icon = "statusarea_cell_level0";
  }
  if (bars_icon != current_bars_icon || mode_icon != current_mode_icon)
  {
    pixbuf = gdk_pixbuf_new(0, 1, 8, 18, 36);
    gdk_pixbuf_fill(pixbuf, 0);
    if (mode_icon)
    {
      mode_icon_pixbuf = connui_pixbuf_cache_get(priv->pixbuf_cache, mode_icon, 11);
      bars_icon_pixbuf = connui_pixbuf_cache_get(priv->pixbuf_cache, bars_icon, 25);
      gdk_pixbuf_composite(bars_icon_pixbuf, pixbuf, 0, 0, 18, 25, 0.0, 0.0, 1.0, 1.0, 0, 255);
      if (mode_icon_pixbuf)
        gdk_pixbuf_composite(mode_icon_pixbuf, pixbuf, 0, 25, 18, 11, 0.0, 25.0, 1.0, 1.0, 0, 255);
    }
    else
    {
      bars_icon_pixbuf2 = connui_pixbuf_cache_get(priv->pixbuf_cache, bars_icon, 25);
      gdk_pixbuf_composite(bars_icon_pixbuf2, pixbuf, 0, 0, 18, 25, 0.0, 0.0, 1.0, 1.0, 0, 255);
    }
    hd_status_plugin_item_set_status_area_icon(HD_STATUS_PLUGIN_ITEM(item), pixbuf);
    g_object_unref((gpointer)pixbuf);
    current_mode_icon = mode_icon;
    current_bars_icon = bars_icon;
  }
}

static void
connui_cellular_status_item_display_cb(osso_display_state_t state,
                                            gpointer *user_data)
{
  ConnuiInternetStatusMenuItem *item =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);

  g_return_if_fail(item != NULL && item->priv != NULL);

  item->priv->display_state = state;
  if (state == OSSO_DISPLAY_ON)
  {
    if (item->priv->needs_display_state_update)
    {
      connui_cellular_status_item_update_icon(item);
      item->priv->needs_display_state_update = FALSE;
    }
  }
}

static void
connui_cellular_status_item_flightmode_cb(gboolean offline, gpointer user_data)
{
  ConnuiInternetStatusMenuItem *item =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);

  g_return_if_fail(item != NULL && item->priv != NULL);

  item->priv->offline = offline;
  connui_cellular_status_item_update_icon(item);
}

static void
connui_cellular_status_item_sim_status_cb(guint status, gpointer user_data)
{
  ConnuiInternetStatusMenuItem *item =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);

  g_return_if_fail(item != NULL && item->priv != NULL);

  item->priv->status = status;
  connui_cellular_status_item_update_icon(item);
}

static void
connui_cellular_status_item_net_status_cb(struct network_state *state, gpointer user_data)
{
  ConnuiInternetStatusMenuItem *item =
      CONNUI_INTERNET_STATUS_MENU_ITEM(user_data);
  if (state)
  {
    item->priv->state->network_reg_status = state->network_reg_status;
    item->priv->state->lac = state->lac;
    item->priv->state->cell_id = state->cell_id;
    item->priv->state->network = state->network;
    item->priv->state->supported_services = state->supported_services;
    item->priv->state->network_signals_bar = state->network_signals_bar;
    item->priv->state->rat = state->rat;
    item->priv->state->network_radio_state = state->network_radio_state;
    item->priv->state->operator_name_type = state->operator_name_type;
    item->priv->state->operator_name = state->operator_name;
    item->priv->state->alternative_operator_name = state->alternative_operator_name;
  }
  else
  {
    item->priv->state->network_reg_status = 0;
    item->priv->state->lac = 0;
    item->priv->state->cell_id = 0;
    item->priv->state->network = 0;
    item->priv->state->supported_services = 0;
    item->priv->state->network_signals_bar = 0;
    item->priv->state->rat = 0;
    item->priv->state->network_radio_state = 0;
    item->priv->state->operator_name_type = 0;
    item->priv->state->operator_name = 0;
    item->priv->state->alternative_operator_name = 0;
  }
  connui_cellular_status_item_update_icon(item);
}

static void
connui_cellular_status_menu_item_finalize(GObject *self)
{
  ConnuiCellularStatusMenuItemPrivate *priv =
      CONNUI_CELLULAR_STATUS_MENU_ITEM(self)->priv;

  if (priv->osso_context)
  {
    osso_deinitialize(priv->osso_context);
    priv->osso_context = 0;
  }

  if (priv->pixbuf_cache)
  {
    connui_pixbuf_cache_destroy(priv->pixbuf_cache);
    priv->pixbuf_cache = 0;
  }

  connui_cell_net_status_close(connui_cellular_status_item_net_status_cb);
  connui_cell_sim_status_close(connui_cellular_status_item_sim_status_cb);
  connui_flightmode_close(connui_cellular_status_item_flightmode_cb);
  
  G_OBJECT_CLASS(connui_cellular_status_menu_item_parent_class)->finalize(self);
}

static void
connui_cellular_status_menu_item_class_init(ConnuiCellularStatusMenuItemClass *klass)
{
  G_OBJECT_CLASS(klass)->finalize = connui_cellular_status_menu_item_finalize;
  g_type_class_add_private(klass, sizeof(ConnuiCellularStatusMenuItemPrivate));
}

static void
connui_cellular_status_menu_item_init(ConnuiCellularStatusMenuItem *self)
{
  ConnuiCellularStatusMenuItemPrivate *priv =
      G_TYPE_INSTANCE_GET_PRIVATE(self,
                                  CONNUI_CELLULAR_STATUS_MENU_ITEM_TYPE,
                                  ConnuiCellularStatusMenuItemPrivate);

  self->priv = priv;
  priv->pixbuf_cache = connui_pixbuf_cache_new();
  priv->osso_context = osso_initialize("connui_cellular_status_item", "2.118+0m5", TRUE, 0);

   osso_hw_set_display_event_cb(
     priv->osso_context,
     (osso_display_event_cb_f *)connui_cellular_status_menu_item_display_cb,
     self);

  if (!connui_flightmode_status(connui_cellular_status_item_flightmode_cb, self))
    g_log(0, G_LOG_LEVEL_WARNING, "Unable to register flightmode status!");
  if (!connui_cell_sim_status_register(connui_cellular_status_item_sim_status_cb, self))
    g_log(0, G_LOG_LEVEL_WARNING, "Unable to register SIM status");
  if (!connui_cell_net_status_register(connui_cellular_status_item_net_status_cb, self))
    g_log(0, G_LOG_LEVEL_WARNING, "Unable to register cell net status!");
  connui_cellular_status_item_update_icon(self);
}

