#include "gui.h"
#include <menu.h>
#include <ncurses.h>
#include <stdio.h>

int gui_init(struct ctx *ctx) {
  initscr();
  refresh();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  getmaxyx(stdscr, ctx->height_max, ctx->width_max);
  nodelay(stdscr, TRUE);
  if (gui_create_window_search_bar(ctx) != 0) {
    return -1;
  }
  if (gui_create_window_description(ctx) != 0) {
    return -1;
  }

  if (gui_create_window_downloads(ctx) != 0) {
    return -1;
  }
  if (gui_create_window_tabs(ctx) != 0) {
    return -1;
  }
  if (gui_create_window_menu(ctx) != 0) {
    return -1;
  }
  return 0;
}

void gui_menu_cleanup(struct ctx *ctx) {
  if (ctx->menu) {
    unpost_menu(ctx->menu);
    free_menu(ctx->menu);
    ctx->menu = NULL;
  }
  ctx->menu = NULL;
  if (ctx->items) {
    for (int i = 0; i < ctx->nb_items; i++) {
      if (ctx->items[i]) {
        free_item(ctx->items[i]);
      }
    }
    free(ctx->items);
    ctx->items = NULL;
    ctx->nb_items = 0;
  }
}

int gui_create_window_search_bar(struct ctx *ctx) {
  WINDOW *win_search_bar =
      newwin(SB_HEIGHT, SB_WIDTH, 2, (ctx->width_max - SB_WIDTH) / 2);
  if (win_search_bar == NULL) {
    fprintf(stderr, "[!] Failed to create window\n");
    return -1;
  }
  ctx->win_search_bar = win_search_bar;
  return 0;
}

int gui_create_window_description(struct ctx *ctx) {
  WINDOW *win_desc = newwin(7, ctx->width_max, ctx->height_max - 8, 0);
  if (win_desc == NULL) {
    fprintf(stderr, "[!] Failed to create window\n");
    return -1;
  }
  ctx->win_description = win_desc;
  return 0;
}

int gui_create_window_tabs(struct ctx *ctx) {
  WINDOW *win_tabs = newwin(1, ctx->width_max, ctx->height_max - 1, 0);
  if (win_tabs == NULL) {
    fprintf(stderr, "[!] Failed to create window\n");
    return -1;
  }
  ctx->win_tabs = win_tabs;
  return 0;
}

int gui_create_window_downloads(struct ctx *ctx) {
  WINDOW *win_downloads = newwin(ctx->height_max - 1, ctx->width_max, 0, 0);
  if (win_downloads == NULL) {
    fprintf(stderr, "[!] Failed to create window\n");
    return -1;
  }
  ctx->win_downloads = win_downloads;
  return 0;
}

int gui_create_window_menu(struct ctx *ctx) {
  WINDOW *win_menu = newwin(ctx->height_max - 8, ctx->width_max, 0, 0);
  if (!win_menu) {
    fprintf(stderr, "[!] Failed to create window\n");
    return -1;
  }
  ctx->win_menu = win_menu;
  return 0;
}

void gui_draw_window_tabs(struct ctx *ctx) {
  if (ctx->current_windows_state == SEARCH) {
    wattr_on(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
    mvwprintw(ctx->win_tabs, 0, 1, "Search");
    wattr_off(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
  } else {
    mvwprintw(ctx->win_tabs, 0, 1, "Search");
  }
  if (ctx->current_windows_state == RESULTS) {
    wattr_on(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
    mvwprintw(ctx->win_tabs, 0, 8, "Results");
    wattr_off(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
  } else {
    mvwprintw(ctx->win_tabs, 0, 8, "Results");
  }
  if (ctx->current_windows_state == DOWNLOADS) {
    wattr_on(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
    mvwprintw(ctx->win_tabs, 0, 16, "Downloads");
    wattr_off(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
  } else {
    mvwprintw(ctx->win_tabs, 0, 16, "Downloads");
  }
}
void gui_draw_window_description(struct ctx *ctx) {
  box(ctx->win_description, 0, 0);
  mvwprintw(ctx->win_description, 0, 1, "Informations");
}

void gui_draw_search_bar(struct ctx *ctx) {
  box(ctx->win_search_bar, 0, 0);
  mvwprintw(ctx->win_search_bar, 0, 1, "search");
  wmove(ctx->win_search_bar, 1, 1);
}

void gui_draw_downloads(struct ctx *ctx) {
  box(ctx->win_downloads, 0, 0);
  mvwprintw(ctx->win_downloads, 0, 1, "Downloads");
}

void gui_draw_menu(struct ctx *ctx) {
  box(ctx->win_menu, 0, 0);
  mvwprintw(ctx->win_menu, 0, 1, "Results");
}

void gui_clear_windows(struct ctx *ctx) {
  if (ctx->current_windows_state == SEARCH) {
    werase(stdscr);
    refresh();
    werase(ctx->win_description);
    wrefresh(ctx->win_description);
    werase(ctx->win_downloads);
    wrefresh(ctx->win_downloads);
  }
  if (ctx->current_windows_state == RESULTS) {
    werase(ctx->win_search_bar);
    wrefresh(ctx->win_search_bar);
    werase(ctx->win_downloads);
    wrefresh(ctx->win_downloads);
  }
  if (ctx->current_windows_state == DOWNLOADS) {
    werase(ctx->win_search_bar);
    wrefresh(ctx->win_search_bar);
    werase(stdscr);
    refresh();
    werase(ctx->win_description);
    wrefresh(ctx->win_description);
  }
}

int gui_create_menu(struct torrent *torrents_list, struct ctx *ctx) {
  int count = 0;
  ctx->nb_items = 0;
  int code_function_return = 0;
  int width, height;
  struct torrent *cp_tl = torrents_list;
  while (cp_tl != NULL) {
    ctx->nb_items++;
    cp_tl = cp_tl->next;
  }
  ctx->items = (ITEM **)calloc((size_t)(ctx->nb_items + 1), sizeof(ITEM *));
  if (!ctx->items) {
    code_function_return = -1;
    goto Cleanup;
  }
  cp_tl = torrents_list;
  while (cp_tl != NULL) {
    ctx->items[count] = new_item(cp_tl->id, cp_tl->information);
    if (!ctx->items[count]) {
      code_function_return = -1;
      goto Cleanup;
    }
    set_item_userptr(ctx->items[count], cp_tl);
    count++;
    cp_tl = cp_tl->next;
  }
  ctx->items[ctx->nb_items] = NULL;
  ctx->menu = new_menu((ITEM **)ctx->items);
  if (!ctx->menu) {
    code_function_return = -1;
    goto Cleanup;
  }
  getmaxyx(ctx->win_menu, height, width);
  WINDOW *sub = derwin(ctx->win_menu, height - 2, width - 2, 1, 1);
  set_menu_win(ctx->menu, ctx->win_menu);
  set_menu_sub(ctx->menu, sub);
  set_menu_format(ctx->menu, MAX_MENU_SIZE, 1);
Cleanup:
  if (code_function_return != 0) {
    if (ctx->items) {
      for (int i = 0; i < ctx->nb_items; i++) {
        if (ctx->items[i]) {
          free_item(ctx->items[i]);
        }
      }
      free(ctx->items);
      ctx->items = NULL;
      ctx->nb_items = 0;
    }
    if (ctx->menu) {
      unpost_menu(ctx->menu);
      free_menu(ctx->menu);
      ctx->menu = NULL;
    }
  }

  return code_function_return;
}

void gui_render(struct ctx *ctx, struct torrent *curr_torrent, char *user_input,
                struct info_download_torrents *active_downloads[MAX_THREAD],
                int count_torrent_currently_downloading) {
  if (ctx->tab_changed == true) {
    /* clear all tabs*/
    werase(ctx->win_search_bar);
    werase(ctx->win_description);
    werase(ctx->win_downloads);
    werase(ctx->win_menu);

    wnoutrefresh(ctx->win_search_bar);
    wnoutrefresh(ctx->win_description);
    wnoutrefresh(ctx->win_downloads);
    wnoutrefresh(ctx->win_menu);
    if (ctx->current_windows_state == SEARCH) {
      box(ctx->win_search_bar, 0, 0);
      mvwprintw(ctx->win_search_bar, 0, 1, "search");
      wmove(ctx->win_search_bar, 1, 1);
      wnoutrefresh(ctx->win_search_bar);
    }
    if (ctx->current_windows_state == RESULTS) {
      /* draw menu */
      mvwprintw(ctx->win_menu, 0, 1, "Results");
      unpost_menu(ctx->menu);
      post_menu(ctx->menu);
      box(ctx->win_menu, 0, 0);
      wnoutrefresh(ctx->win_menu);
      /* draw description */
      box(ctx->win_description, 0, 0);
      mvwprintw(ctx->win_description, 0, 1, "Informations");
      if (curr_torrent->information) {
        mvwprintw(ctx->win_description, 1, 1,
                  " Description: %s \n seeders:%s\n size:%s",
                  curr_torrent->information, curr_torrent->seeders,
                  curr_torrent->size);
      }
      wnoutrefresh(ctx->win_description);
    }
    /* draw tabs */
    if (ctx->current_windows_state == SEARCH) {
      wattr_on(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
      mvwprintw(ctx->win_tabs, 0, 1, "Search");
      wattr_off(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
    } else {
      mvwprintw(ctx->win_tabs, 0, 1, "Search");
    }
    if (ctx->current_windows_state == RESULTS) {
      wattr_on(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
      mvwprintw(ctx->win_tabs, 0, 8, "Results");
      wattr_off(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
    } else {
      mvwprintw(ctx->win_tabs, 0, 8, "Results");
    }
    if (ctx->current_windows_state == DOWNLOADS) {
      wattr_on(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
      mvwprintw(ctx->win_tabs, 0, 16, "Downloads");
      wattr_off(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
    } else {
      mvwprintw(ctx->win_tabs, 0, 16, "Downloads");
    }
    wnoutrefresh(ctx->win_tabs);
    ctx->tab_changed = false;
  }
  if (ctx->dirty_menu) {
    box(ctx->win_menu, 0, 0);
    mvwprintw(ctx->win_menu, 0, 1, "Results");
    wnoutrefresh(ctx->win_menu);
    ctx->dirty_menu = false;
  }
  if (ctx->dirty_search) {
    werase(ctx->win_search_bar);
    box(ctx->win_search_bar, 0, 0);
    mvwprintw(ctx->win_search_bar, 0, 1, "search");
    wmove(ctx->win_search_bar, 1, 1);
    wprintw(ctx->win_search_bar, "%s", user_input);
    ctx->dirty_search = false;
    wnoutrefresh(ctx->win_search_bar);
  }
  if (ctx->dirty_description) {
    werase(ctx->win_description);
    box(ctx->win_description, 0, 0);
    mvwprintw(ctx->win_description, 0, 1, "Informations");
    mvwprintw(
        ctx->win_description, 1, 1, " Description: %s \n seeders:%s\n size:%s",
        curr_torrent->information, curr_torrent->seeders, curr_torrent->size);
    ctx->dirty_description = false;
    wnoutrefresh(ctx->win_description);
  }
  if (ctx->current_windows_state == DOWNLOADS) {
    werase(ctx->win_downloads);
    box(ctx->win_downloads, 0, 0);
    mvwprintw(ctx->win_downloads, 0, 1, "Downloads");
    int index = 0;
    for (int i = 0; i < count_torrent_currently_downloading; i++) {
      mvwprintw(ctx->win_downloads, (index), 0,
                "%s :", active_downloads[i]->name);
      mvwprintw(ctx->win_downloads, (index + 1), 0, "%s",
                active_downloads[i]->upload_progress);
      mvwprintw(ctx->win_downloads, (index + 2), 0, "%s",
                active_downloads[i]->download_progress);
      index += 4;
    }
    wnoutrefresh(ctx->win_downloads);
  }
  doupdate();
}
