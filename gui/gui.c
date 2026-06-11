#include "gui.h"

/* ---------------- INIT NCURSES ---------------- */

int guiInit(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    curs_set(1);

    start_color();

    init_pair(1, COLOR_BLACK, COLOR_CYAN);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);

    return 0;
}

/* ---------------- WINDOWS ---------------- */

WINDOW *guiCreateSearchBar(void)
{
    WINDOW *win = newwin(3, COLS - 4, 1, 2);
    box(win, 0, 0);
    wrefresh(win);
    return win;
}

WINDOW *guiCreateMenuWindow(void)
{
    WINDOW *win = newwin(LINES - 6, COLS - 4, 5, 2);
    box(win, 0, 0);
    wrefresh(win);
    return win;
}

/* ---------------- MENU ---------------- */

int guiCreateMenu(GuiContext *ctx, char **choices, int n_choices)
{
    ctx->items = calloc(n_choices + 1, sizeof(ITEM *));

    for (int i = 0; i < n_choices; i++)
        ctx->items[i] = new_item(choices[i], "");

    ctx->items[n_choices] = NULL;

    ctx->menu = new_menu(ctx->items);

    ctx->menu_win = guiCreateMenuWindow();

    set_menu_win(ctx->menu, ctx->menu_win);

    ctx->menu_sub =
        derwin(ctx->menu_win,
               LINES - 10,
               COLS - 6,
               1, 1);

    set_menu_sub(ctx->menu, ctx->menu_sub);

    set_menu_mark(ctx->menu, " > ");

    set_menu_fore(ctx->menu, COLOR_PAIR(1) | A_BOLD);
    set_menu_back(ctx->menu, COLOR_PAIR(2));

    post_menu(ctx->menu);

    return 0;
}

/* rebuild menu dynamically later */
void guiRebuildMenu(GuiContext *ctx, char **choices, int n)
{
    unpost_menu(ctx->menu);
    free_menu(ctx->menu);

    for (int i = 0; ctx->items && ctx->items[i]; i++)
        free_item(ctx->items[i]);

    free(ctx->items);

    guiCreateMenu(ctx, choices, n);
}

/* ---------------- DRAW ---------------- */

void guiDrawSearchBar(GuiContext *ctx)
{
    werase(ctx->search_win);
    box(ctx->search_win, 0, 0);

    mvwprintw(ctx->search_win,
              1,
              2,
              "Search: %s",
              ctx->search_buffer);

    wrefresh(ctx->search_win);
}

void guiDrawMenu(GuiContext *ctx)
{
    box(ctx->menu_win, 0, 0);
    wrefresh(ctx->menu_win);
}

/* ---------------- INPUT ---------------- */

void guiHandleInput(GuiContext *ctx, int key)
{
    int len = strlen(ctx->search_buffer);

    switch (key)
    {
        case KEY_UP:
            menu_driver(ctx->menu, REQ_UP_ITEM);
            break;

        case KEY_DOWN:
            menu_driver(ctx->menu, REQ_DOWN_ITEM);
            break;

        case KEY_BACKSPACE:
        case 127:
            if (len > 0)
                ctx->search_buffer[len - 1] = '\0';
            break;

        case 10: /* Enter */
            /* future: trigger API search */
            break;

        default:
            if (key >= 32 && key <= 126)
            {
                if (len < SEARCH_MAX - 1)
                {
                    ctx->search_buffer[len] = key;
                    ctx->search_buffer[len + 1] = '\0';
                }
            }
            break;
    }
}

/* ---------------- SETUP ---------------- */

int guiSetup(GuiContext *ctx, char **choices, int n_choices)
{
    memset(ctx, 0, sizeof(GuiContext));

    guiInit();

    ctx->search_win = guiCreateSearchBar();

    guiCreateMenu(ctx, choices, n_choices);

    return 0;
}

/* ---------------- LOOP ---------------- */

void guiEventLoop(GuiContext *ctx)
{
    int ch;

    while ((ch = getch()) != KEY_F(1))
    {
        guiHandleInput(ctx, ch);

        guiDrawSearchBar(ctx);

        wrefresh(ctx->menu_sub);
    }
}

/* ---------------- CLEANUP ---------------- */

void guiCleanup(GuiContext *ctx, int n_choices)
{
    unpost_menu(ctx->menu);
    free_menu(ctx->menu);

    for (int i = 0; i < n_choices; i++)
        free_item(ctx->items[i]);

    free(ctx->items);

    delwin(ctx->menu_sub);
    delwin(ctx->menu_win);
    delwin(ctx->search_win);

    endwin();
}