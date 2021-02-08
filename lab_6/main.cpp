#include <string>
#include <queue>
#include <cstring>
#include <iostream>
#include <gtk/gtk.h>

/***** global functions *****/
static void initial_handlers(void);
static void button_start(void);
static void parse_text_buffer(void);
static void init(void);

/***** global variables *****/
std::queue<std::string> queue;

/***** GTK *****/
GtkBuilder *builder;
GtkWindow *window;
GtkTextBuffer *text_input;
GtkTextBuffer *text_output;

int main(int argc, char *argv[])
{
    /***** GTK *****/
    gtk_init(&argc, &argv);
    
    init();

    /***** start *****/
    gtk_main();
    return 0;
}
static void
init(void)
{
    GError *error = NULL;
    builder = gtk_builder_new();
    
    if (gtk_builder_add_from_file(builder, "main.glade", &error) == 0)
    {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);
    }

    window = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
    text_input = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "text_buffer_1"));
    text_output = GTK_TEXT_BUFFER(gtk_builder_get_object(builder, "text_buffer_2"));
    
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    initial_handlers();
}
static void
initial_handlers(void)
{
    GObject *button;
    button = gtk_builder_get_object(builder, "but_start");
    g_signal_connect(button, "clicked", G_CALLBACK(button_start), NULL);
}
static void
button_start(void)
{
    try
    {
        int amount_lines;
        std::string result = "";
        std::string temp = "";
        parse_text_buffer();
        amount_lines = gtk_text_buffer_get_line_count(text_input);
        for (int i = 1; !queue.empty(); i++)
        {
            if (i % 2 == 0)
                result += queue.front();
            else
                temp += queue.front();
            queue.pop();
        }
        result += temp;
        gtk_text_buffer_set_text(text_output, result.c_str(), result.size());
    }
    catch (const char *msg)
    {
        g_printerr("Error: %s\n", msg);
    }
    catch (...)
    {
        g_printerr("Error: something is wrong.");
    }
}
static void
parse_text_buffer(void)
{
    /***** read from text_buffer_1 *****/
    const gint count_lines = gtk_text_buffer_get_char_count(text_input);
    GtkTextIter start, end;
    gchar *buffer;

    gtk_text_buffer_get_start_iter(text_input, &start);
    gtk_text_buffer_get_end_iter(text_input, &end);
    gtk_text_buffer_begin_user_action(text_input);
    buffer = gtk_text_iter_get_text(&start, &end);
    gtk_text_buffer_end_user_action(text_input);

    std::string temp = "";
    gchar c;
    for (int i = 0; c = buffer[i]; i++)
    {
        temp += c;
        if (c == '\n')
        {
            queue.push(temp);
            temp.clear();
        }
    }
    if (temp.size() > 0) /* на тот случай, если в последней строке не было '\n' */
    {
        queue.push(temp);
        temp.clear();
    }
}