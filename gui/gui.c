#include "gui.h"
#include <menu.h>
#include <ncurses.h>
#include <stdio.h>
#include <locale.h>

int gui_init(struct ctx *ctx) {
  setlocale(LC_ALL, "en_US.UTF-8");
  initscr();
  cbreak();
  noecho();
  curs_set(0);
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
  if(gui_create_window_menu_downloaded_movies(ctx) != 0){
    return -1;
  }
  return 0;
}

void gui_menu_cleanup(struct menu_objects *menu_obj) {
  if (menu_obj->menu) {
    unpost_menu(menu_obj->menu);
    free_menu(menu_obj->menu);
    menu_obj->menu = NULL;
  }
  if (menu_obj->items) {
    for (int i = 0; i < menu_obj->nb_items; i++) {
      if (menu_obj->items[i]) {
        free_item(menu_obj->items[i]);
      }
    }
    free(menu_obj->items);
    menu_obj->items = NULL;
    menu_obj->nb_items = 0;
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

int gui_create_window_menu_downloaded_movies(struct ctx *ctx) {
  WINDOW *win_menu_downloaded_movies = newwin(ctx->height_max - 1, ctx->width_max, 0, 0);
  if (!win_menu_downloaded_movies) {
    fprintf(stderr, "[!] Failed to create window\n");
    return -1;
  }
  ctx->win_menu_downloaded_movies = win_menu_downloaded_movies;
  return 0;
}

int gui_create_menu(struct torrent *torrents_list, struct ctx *ctx) {
  int count = 0;
  ctx->menu_obj_torrents_result.nb_items = 0;
  int code_function_return = 0;
  int width, height;
  struct torrent *cp_tl = torrents_list;
  while (cp_tl != NULL) {
    ctx->menu_obj_torrents_result.nb_items++;
    cp_tl = cp_tl->next;
  }
  ctx->menu_obj_torrents_result.items = (ITEM **)calloc((size_t)(ctx->menu_obj_torrents_result.nb_items + 1), sizeof(ITEM *));
  if (!ctx->menu_obj_torrents_result.nb_items) {
    code_function_return = -1;
    goto cleanup;
  }
  cp_tl = torrents_list;
  while (cp_tl != NULL) {
    ctx->menu_obj_torrents_result.items[count] = new_item(cp_tl->id, cp_tl->information);
    if (!ctx->menu_obj_torrents_result.items[count]) {
      code_function_return = -1;
      goto cleanup;
    }
    set_item_userptr(ctx->menu_obj_torrents_result.items[count], cp_tl);
    count++;
    cp_tl = cp_tl->next;
  }
  ctx->menu_obj_torrents_result.items[ctx->menu_obj_torrents_result.nb_items] = NULL;
  ctx->menu_obj_torrents_result.menu = new_menu((ITEM **)ctx->menu_obj_torrents_result.items);
  if (!ctx->menu_obj_torrents_result.menu) {
    code_function_return = -1;
    goto cleanup;
  }
  getmaxyx(ctx->win_menu, height, width);
  WINDOW *sub = derwin(ctx->win_menu, height - 2, width - 2, 1, 1);
  set_menu_win(ctx->menu_obj_torrents_result.menu, ctx->win_menu);
  set_menu_sub(ctx->menu_obj_torrents_result.menu, sub);
  set_menu_format(ctx->menu_obj_torrents_result.menu, MAX_MENU_SIZE, 1);

  cleanup:
    if (code_function_return != 0) {
      if (ctx->menu_obj_torrents_result.items) {
        for (int i = 0; i < ctx->menu_obj_torrents_result.nb_items; i++) {
          if (ctx->menu_obj_torrents_result.items[i]) {
            free_item(ctx->menu_obj_torrents_result.items[i]);
          }
        }
        free(ctx->menu_obj_torrents_result.items);
        ctx->menu_obj_torrents_result.items = NULL;
        ctx->menu_obj_torrents_result.nb_items = 0;
      }
      if (ctx->menu_obj_torrents_result.menu) {
        unpost_menu(ctx->menu_obj_torrents_result.menu);
        free_menu(ctx->menu_obj_torrents_result.menu);
        ctx->menu_obj_torrents_result.menu = NULL;
      }
    }

  return code_function_return;
}

int gui_create_downloaded_movies_menu(struct downloaded_movies_list *movies_list, struct ctx *ctx){
  int code_function_return = 0;
  int width,height;
  if(!movies_list){
    code_function_return = -1;
    goto cleanup;
  }
  ctx->menu_obj_movies_downloaded.nb_items = movies_list->number_of_movies;
  ctx->menu_obj_movies_downloaded.items = (ITEM **)calloc((size_t)(ctx->menu_obj_movies_downloaded.nb_items + 1), sizeof(ITEM *));
  for(int i=0; i<ctx->menu_obj_movies_downloaded.nb_items;i++){
    ctx->menu_obj_movies_downloaded.items[i] = new_item((const char*)movies_list->movies_list_name_cleaned[i],NULL);
    set_item_userptr(ctx->menu_obj_movies_downloaded.items[i],movies_list->movies_list[i]);
  }
  ctx->menu_obj_movies_downloaded.items[ctx->menu_obj_movies_downloaded.nb_items] = NULL;
  ctx->menu_obj_movies_downloaded.menu = new_menu((ITEM **)ctx->menu_obj_movies_downloaded.items);
  if (!ctx->menu_obj_movies_downloaded.menu) {
    code_function_return = -1;
    goto cleanup;
  }
  getmaxyx(ctx->win_menu_downloaded_movies, height, width);
  WINDOW *sub = derwin(ctx->win_menu_downloaded_movies, height - 2, width - 2, 1, 1);
  set_menu_win(ctx->menu_obj_movies_downloaded.menu, ctx->win_menu_downloaded_movies);
  set_menu_sub(ctx->menu_obj_movies_downloaded.menu, sub);
  set_menu_format(ctx->menu_obj_movies_downloaded.menu, MAX_MENU_SIZE_DOWNLOADED_MOVIES, 1);

  cleanup:
    if(code_function_return != 0){
      if (ctx->menu_obj_movies_downloaded.items) {
        for (int i = 0; i < ctx->menu_obj_movies_downloaded.nb_items; i++) {
          if (ctx->menu_obj_movies_downloaded.items[i]) {
            free_item(ctx->menu_obj_movies_downloaded.items[i]);
          }
        }
        free(ctx->menu_obj_movies_downloaded.items);
        ctx->menu_obj_movies_downloaded.items = NULL;
        ctx->menu_obj_movies_downloaded.nb_items = 0;
      }
      if (ctx->menu_obj_movies_downloaded.menu) {
        unpost_menu(ctx->menu_obj_movies_downloaded.menu);
        free_menu(ctx->menu_obj_movies_downloaded.menu);
        ctx->menu_obj_movies_downloaded.menu = NULL;
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
    werase(ctx->win_menu_downloaded_movies);

    wnoutrefresh(ctx->win_search_bar);
    wnoutrefresh(ctx->win_description);
    wnoutrefresh(ctx->win_downloads);
    wnoutrefresh(ctx->win_menu);
    wnoutrefresh(ctx->win_menu_downloaded_movies);
    if (ctx->current_windows_state == SEARCH) {
      box(ctx->win_search_bar, 0, 0);
      mvwprintw(ctx->win_search_bar, 0, 1, "search");
      wmove(ctx->win_search_bar, 1, 1);
      wnoutrefresh(ctx->win_search_bar);
    }
    if (ctx->current_windows_state == RESULTS) {
      /* draw menu */
      if(ctx->is_result){
        unpost_menu(ctx->menu_obj_torrents_result.menu);
        post_menu(ctx->menu_obj_torrents_result.menu);
        box(ctx->win_menu, 0, 0);
        mvwprintw(ctx->win_menu, 0, 1, "Results");
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
    if (ctx->current_windows_state == DOWNLOADED_MOVIES) {
      wattr_on(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
      mvwprintw(ctx->win_tabs, 0, 26, "Downloaded movies");
      wattr_off(ctx->win_tabs, (A_BOLD | A_UNDERLINE), NULL);
    } else {
      mvwprintw(ctx->win_tabs, 0, 26, "Downloaded movies");
    }
    wnoutrefresh(ctx->win_tabs);
    ctx->tab_changed = false;
  }
  /* Redraw modified items */
  if (ctx->dirty_menu) {
    wnoutrefresh(ctx->win_menu);
    ctx->dirty_menu = false;
  }
  if (ctx->dirty_menu_downloaded_movies) {
    wnoutrefresh(ctx->win_menu_downloaded_movies);
    ctx->dirty_menu_downloaded_movies = false;
  }
  if (ctx->dirty_search) {
    werase(ctx->win_search_bar);
    box(ctx->win_search_bar, 0, 0);
    mvwprintw(ctx->win_search_bar, 0, 1, "search");
    mvwprintw(ctx->win_search_bar,1,1,"%s", user_input);
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
      mvwprintw(ctx->win_downloads, (index+1), 0,
                "%s :", active_downloads[i]->name);
      mvwprintw(ctx->win_downloads, (index + 2), 1, "%s",active_downloads[i]->upload_progress);
      mvwprintw(ctx->win_downloads, (index + 3), 1, "%s",active_downloads[i]->download_progress);
      index += 4;
    }
    wnoutrefresh(ctx->win_downloads);
  }
  if(ctx->current_windows_state == DOWNLOADED_MOVIES){
    /* draw menu */
    unpost_menu(ctx->menu_obj_movies_downloaded.menu);
    post_menu(ctx->menu_obj_movies_downloaded.menu);
    box(ctx->win_menu_downloaded_movies, 0, 0);
    mvwprintw(ctx->win_menu_downloaded_movies, 0, 1, "Downloaded movies");
    wnoutrefresh(ctx->win_menu_downloaded_movies);
  }
  if (ctx->current_windows_state == SEARCH) {
    wmove(ctx->win_search_bar, 1, 1 + (int)strlen(user_input));
    wnoutrefresh(ctx->win_search_bar);   
    curs_set(1);
  } else {
    curs_set(0);
  }
  doupdate();
}
