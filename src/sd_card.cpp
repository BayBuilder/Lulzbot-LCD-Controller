#include <Arduino.h>
#include <config.h>
#include <sd_card.h>
#include <SdFat.h>
#include <ui.h>

SdFat sd;

// ============================================================
// SD SPI object
static SPIClass sdSPI(VSPI);


/* --- SD Initialization --- */
bool sd_init()
{
    LOG("Initializing SD...");

    // Make sure SD is deselected.
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    // Configure VSPI for SD.
    sdSPI.begin(
        SD_SCK,  // SCK 18
        SD_MISO, // MISO 19
        SD_MOSI, // MOSI 23
        SD_CS    // CS 5
    );

    SdSpiConfig sdConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(20), &sdSPI);

    if (!sd.begin(sdConfig))
    {
        LOG("SD initialization FAILED");
        return false;
    }

    LOG("SD initialization OK");
    LOGN("SD card size: ");
    LOGN(sd.card()->sectorCount() / 2048);
    LOG(" MB");
    return true;
}

/* --- SD File Browser --- */

// The file browser is a path header and a table of file and 
// directory entries read using the `SdFat` library.
// The static helpers below build one on top of base widgets

static lv_obj_t * browser;
static lv_obj_t * browser_path_label;
static lv_obj_t * browser_table;
static char browser_path[SD_MAX_PATH_LEN];
char selected_file[SD_MAX_PATH_LEN] = "";

// G-code extensions accepted by the file browser
#define GCODE_EXTENSION_COUNT 5
static const char *gcodeExtensions[GCODE_EXTENSION_COUNT] =
{
    "gcode",
    "gco",
    "g",
    "gc",
    "nc"
};

static bool isGcodeFile(const char *filename)
{
    const char *dot = strrchr( filename, '.');

    if (dot == nullptr)
        return false;

    dot++;

    for (uint8_t i = 0; i < GCODE_EXTENSION_COUNT; i++) {
        if (strcasecmp( dot, gcodeExtensions[i]) == 0) {
            return true;
        }
    }

    return false;
}

// Join a direcctory path and filename into a single unit
// Return false if the result is too big for the return buffer
static bool path_join(char * buf, size_t buf_size, const char * base, const char * name)
{
    int res = lv_fs_path_join(buf, buf_size, base, name);
    return res >= 0 && (size_t)res < buf_size;
}

// Fill table with entries from the current path
static void browser_open(const char * path)
{
    FsFile dir;
    FsFile file;

    // Makae sure the first row shows up
    // It goes to the parent directory or closes
    // the browser if at the root
    uint32_t row = 0;
    lv_table_set_row_count(browser_table, 0);
    lv_table_set_cell_value(browser_table, row, 0, LV_SYMBOL_LEFT);
    lv_table_set_cell_value(browser_table, row, 1, "..");
    row++;

    // Open the current directory
    if(!dir.open(path)) {
        
        lv_label_set_text(browser_path_label, "SD card error");
        return;
    }

    lv_snprintf(browser_path, sizeof(browser_path), "%s", path);
    lv_label_set_text(browser_path_label, browser_path);

    
    char fn[SD_MAX_PATH_LEN];

    // Add directories to beginning of browser_table
    while (file.openNext(&dir, O_RDONLY)) {

      if (file.isDir()) {
        file.getName(fn, sizeof(fn));
        lv_table_set_cell_value(browser_table, row, 0, LV_SYMBOL_DIRECTORY);
        lv_table_set_cell_value(browser_table, row, 1, fn);
        row++;
      }
      file.close();
    }

    // Add files after directories
    dir.rewindDirectory();
    while (file.openNext(&dir, O_RDONLY)) {

      if (file.isFile()) {
        file.getName(fn, sizeof(fn));
        if (isGcodeFile(fn)) {
        lv_table_set_cell_value(browser_table, row, 0, LV_SYMBOL_FILE);
        lv_table_set_cell_value(browser_table, row, 1, fn);
        row++;
        }
      }
      file.close();
    }

    if(dir.getError()) {
      LOGN("Error opening ");
      LOGN(path);
      LOG(", browser listing incomplete");
    }

    dir.close();
}

// Navigate to the parent directory by dropping the last path component
static void browser_up(void)
{
    char buf[SD_MAX_PATH_LEN];
    lv_snprintf(buf, sizeof(buf), "%s", browser_path);

    size_t len = strlen(buf);
    if(len > 0 && buf[len - 1] == '/') buf[len - 1] = '\0';   /*drop a trailing slash*/

    char * slash = strrchr(buf, '/');
    if(slash == NULL) {
      // Already at the drive root - close the browser
      lv_obj_delete(browser);
      return;
    }
    slash[1] = '\0';                 /*keep the slash, cut the last component*/
    browser_open(buf);
}

// An item in the file browser table was clicked
static void table_event(lv_event_t * e)
{
    LV_UNUSED(e);
    uint32_t row, col;
    lv_table_get_selected_cell(browser_table, &row, &col);
    if(row == LV_TABLE_CELL_NONE) return;

    const char * icon = lv_table_get_cell_value(browser_table, row, 0);
    const char * name = lv_table_get_cell_value(browser_table, row, 1);

    // ".." was clicked, go up one directory level
    if(strcmp(name, "..") == 0) {
        browser_up();
        return;
    }

    // A file or directory was clicked, add it to the path
    char child[SD_MAX_PATH_LEN];
    if(!path_join(child, sizeof(child), browser_path, name)) {
        LOGN("Path of ");
        LOGN(name);
        LOG(" is too long");
        return;
    }

    // For a file, set the resulting filename, enable the UI print button,
    // and close the browser
    if(strcmp(icon, LV_SYMBOL_DIRECTORY) != 0) {
        snprintf(selected_file, sizeof(selected_file), child);
        lv_label_set_text(fileLabel, selected_file);
        lv_obj_delete(browser);
        enable_print_btn();
        LOGN("Selected file: ");
        LOG(child);
        return;
    }

    // For a directory, navigate into the directory
    browser_open(child);
}

// File browser 
//
// The white browser area shows the current path over an `lv_table`
// filled from a directory read with the SdFat library.
// Clicking a folder opens the child directory, the ".." row walks back up.
void sd_file_browser(lv_obj_t * page)
{
    // Browser covers the entire tab 
    browser = lv_obj_create(page);
    lv_obj_set_size(browser, lv_pct(100), lv_pct(100));
    lv_obj_move_foreground(browser); // Stay on top of other widgets
    lv_obj_set_flex_grow(browser, 1);
    lv_obj_set_flex_flow(browser, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_style(browser, &style_panel, 0);

    // Create the directory path bar on top
    browser_path_label = lv_label_create(browser);
    lv_obj_set_width(browser_path_label, lv_pct(100));
    lv_label_set_long_mode(browser_path_label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_add_style(browser_path_label, &style_header, 0);

    // Create the table below
    browser_table = lv_table_create(browser);
    lv_obj_set_width(browser_table, lv_pct(100));
    lv_obj_set_flex_grow(browser_table, 1);
    lv_obj_add_style(browser_table, &style_table, 0);
    // Tighten the cell padding so the icons don't get clipped
    lv_obj_set_style_pad_hor(browser_table, 6, LV_PART_ITEMS);
    lv_obj_set_style_pad_ver(browser_table, 8, LV_PART_ITEMS);

    // Icon column fits a symbol; name column fills the rest of the browser area
    int32_t icon_col = 30;
    int32_t name_col = lv_obj_get_width(page) - icon_col - 16;
    lv_table_set_column_count(browser_table, 2);
    lv_table_set_column_width(browser_table, 0, icon_col);
    lv_table_set_column_width(browser_table, 1, name_col);
    lv_obj_add_event_cb(browser_table, table_event, LV_EVENT_VALUE_CHANGED, NULL);

    // Start browsing at "/"
    browser_open(BROWSER_ROOT);
}

#if 0

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <SdFat.h>

static SdFat sd;


static lv_obj_t *file_dropdown;
static lv_obj_t *start_button;
static lv_obj_t *pause_button;
static lv_obj_t *stop_button;
static lv_obj_t *progress_bar;

static lv_obj_t *status_label;
static lv_obj_t *line_label;
static lv_obj_t *command_label;


// ============================================================
// GCODE SENDER STATE
// ============================================================

enum SenderState
{
    SENDER_IDLE,
    SENDER_RUNNING,
    SENDER_PAUSED,
    SENDER_STOPPING,
    SENDER_ERROR
};

static SenderState sender_state = SENDER_IDLE;

static FsFile gcode_file;

static uint32_t total_lines = 0;
static uint32_t current_line = 0;

static uint32_t marlin_line_number = 1;

static char current_command[256];



// ============================================================
// UI HELPERS
// ============================================================

static void set_status(const char *text)
{
    lv_label_set_text(
        status_label,
        text
    );
}


static void update_progress()
{
    if (total_lines == 0)
    {
        lv_bar_set_value(
            progress_bar,
            0,
            LV_ANIM_OFF
        );
    }
    else
    {
        uint32_t percent =
            (current_line * 100UL) /
            total_lines;

        if (percent > 100)
            percent = 100;

        lv_bar_set_value(
            progress_bar,
            percent,
            LV_ANIM_OFF
        );
    }

    char text[64];

    snprintf(
        text,
        sizeof(text),
        "Line %lu / %lu",
        (unsigned long)current_line,
        (unsigned long)total_lines
    );

    lv_label_set_text(
        line_label,
        text
    );
}


static void update_command()
{
    lv_label_set_text(
        command_label,
        current_command
    );
}


// ============================================================
// COUNT FILE LINES
// ============================================================

static uint32_t count_lines(FsFile &file)
{
    uint32_t count = 0;

    file.seek(0);

    bool got_data = false;

    int c;

    while ((c = file.read()) >= 0)
    {
        got_data = true;

        if (c == '\n')
            count++;
    }

    if (got_data)
        count++;

    file.seek(0);

    return count;
}


// ============================================================
// READ ONE GCODE LINE
// ============================================================

static bool read_gcode_line(
    FsFile &file,
    char *buffer,
    size_t buffer_size)
{
    size_t pos = 0;

    while (file.available())
    {
        int c = file.read();

        if (c < 0)
            break;

        if (c == '\r')
            continue;

        if (c == '\n')
            break;

        if (pos < buffer_size - 1)
        {
            buffer[pos++] =
                (char)c;
        }
    }

    buffer[pos] = '\0';

    return pos > 0;
}


// ============================================================
// STRIP GCODE COMMENTS
// ============================================================

static void strip_comments(char *line)
{
    bool in_parentheses = false;

    char *src = line;
    char *dst = line;

    while (*src)
    {
        if (*src == '(')
        {
            in_parentheses = true;
            src++;
            continue;
        }

        if (*src == ')')
        {
            in_parentheses = false;
            src++;
            continue;
        }

        if (!in_parentheses &&
            *src == ';')
        {
            break;
        }

        if (!in_parentheses)
        {
            *dst++ = *src;
        }

        src++;
    }

    *dst = '\0';
}


// ============================================================
// TRIM LEADING AND TRAILING WHITESPACE
// ============================================================

static void trim_line(char *line)
{
    char *start = line;

    while (*start == ' ' ||
           *start == '\t')
    {
        start++;
    }

    if (start != line)
        memmove(
            line,
            start,
            strlen(start) + 1
        );

    size_t len =
        strlen(line);

    while (len > 0)
    {
        char c = line[len - 1];

        if (c == ' ' ||
            c == '\t')
        {
            line[--len] = '\0';
        }
        else
        {
            break;
        }
    }
}


// ============================================================
// MARLIN CHECKSUM
// ============================================================
//
// Marlin checksum:
//
//   XOR every character in:
//
//       N123 G1 X10 Y20
//
// Do NOT include the '*' or checksum itself.
//
// ============================================================

static uint8_t marlin_checksum(
    const char *line)
{
    uint8_t checksum = 0;

    while (*line)
    {
        checksum ^= (uint8_t)*line++;
    }

    return checksum;
}


// ============================================================
// BUILD MARLIN COMMAND
// ============================================================

static bool build_marlin_command(
    const char *gcode,
    char *output,
    size_t output_size)
{
    int written = snprintf(
        output,
        output_size,
        "N%lu %s",
        (unsigned long)marlin_line_number,
        gcode
    );

    if (written <= 0 ||
        written >= output_size)
    {
        return false;
    }

    uint8_t checksum =
        marlin_checksum(output);

    size_t len =
        strlen(output);

    snprintf(
        output + len,
        output_size - len,
        "*%u",
        checksum
    );

    return true;
}


// ============================================================
// SEND ONE COMMAND TO MARLIN
// ============================================================

static void send_command(
    const char *command)
{
    Serial.print(F("TX: "));
    Serial.println(command);

    MarlinSerial.println(command);
}


// ============================================================
// WAIT FOR MARLIN OK
// ============================================================
//
// This is deliberately kept simple here.
//
// The production version should maintain a receive state
// machine capable of handling:
//
//   ok
//   Resend:N
//   Error:
//   echo:
//   busy:
//
// without blocking the UI.
//
// ============================================================

static bool wait_for_ok(
    uint32_t timeout_ms)
{
    uint32_t start =
        millis();

    char response[128];

    size_t pos = 0;

    while (millis() - start <
           timeout_ms)
    {
        while (MarlinSerial.available())
        {
            char c =
                MarlinSerial.read();

            if (c == '\r')
                continue;

            if (c == '\n')
            {
                response[pos] =
                    '\0';

                Serial.print(F("RX: "));
                Serial.println(response);

                if (strstr(
                        response,
                        "ok"))
                {
                    return true;
                }

                if (strstr(
                        response,
                        "Resend:"))
                {
                    return false;
                }

                pos = 0;
            }
            else if (pos <
                     sizeof(response) - 1)
            {
                response[pos++] = c;
            }
        }

        lv_timer_handler();

        delay(1);
    }

    return false;
}


// ============================================================
// OPEN SELECTED FILE
// ============================================================

static bool open_selected_file()
{
    char filename[128];

    lv_dropdown_get_selected_str(
        file_dropdown,
        filename,
        sizeof(filename)
    );

    if (filename[0] == '\0')
        return false;

    char path[160];

    snprintf(
        path,
        sizeof(path),
        "/%s",
        filename
    );

    // Close any existing open file
    if (gcode_file)
        gcode_file.close();

    gcode_file =
        sd.open(
            path,
            FILE_READ
        );

    if (!gcode_file)
    {
        Serial.print(
            F("Cannot open: ")
        );

        Serial.println(path);

        set_status(
            "Cannot open file"
        );

        return false;
    }

    total_lines =
        count_lines(gcode_file);

    current_line = 0;

    marlin_line_number = 1;

    Serial.print(
        F("Opened: ")
    );

    Serial.println(path);

    Serial.print(
        F("Lines: ")
    );

    Serial.println(total_lines);

    return true;
}


// ============================================================
// START SENDER
// ============================================================

static void start_sender()
{
    if (sender_state ==
            SENDER_RUNNING)
    {
        return;
    }

    if (!gcode_file)
    {
        if (!open_selected_file())
            return;
    }

    sender_state =
        SENDER_RUNNING;

    set_status(
        "Sending..."
    );

    Serial.println(
        F("=== GCODE START ===")
    );
}


// ============================================================
// PAUSE
// ============================================================

static void pause_sender()
{
    if (sender_state !=
        SENDER_RUNNING)
    {
        return;
    }

    sender_state =
        SENDER_PAUSED;

    set_status(
        "Paused"
    );

    Serial.println(
        F("=== GCODE PAUSED ===")
    );
}


// ============================================================
// STOP
// ============================================================

static void stop_sender()
{
    if (sender_state ==
        SENDER_IDLE)
    {
        return;
    }

    sender_state =
        SENDER_STOPPING;

    Serial.println(
        F("=== GCODE STOP ===")
    );

    if (gcode_file)
        gcode_file.close();

    sender_state =
        SENDER_IDLE;

    current_line = 0;

    update_progress();

    set_status(
        "Stopped"
    );
}


// ============================================================
// PROCESS ONE GCODE LINE
// ============================================================

static void process_gcode()
{
    if (sender_state !=
        SENDER_RUNNING)
    {
        return;
    }

    if (!gcode_file)
    {
        sender_state =
            SENDER_ERROR;

        set_status(
            "File not open"
        );

        return;
    }


    // --------------------------------------------------------
    // End of file
    // --------------------------------------------------------

    if (!gcode_file.available())
    {
        gcode_file.close();

        sender_state =
            SENDER_IDLE;

        lv_bar_set_value(
            progress_bar,
            100,
            LV_ANIM_OFF
        );

        set_status(
            "Complete"
        );

        Serial.println(
            F("=== GCODE COMPLETE ===")
        );

        return;
    }


    // --------------------------------------------------------
    // Read line
    // --------------------------------------------------------

    char gcode[192];

    if (!read_gcode_line(
            gcode_file,
            gcode,
            sizeof(gcode)))
    {
        return;
    }

    current_line++;

    strip_comments(gcode);
    trim_line(gcode);

    // Empty/comment-only line
    if (gcode[0] == '\0')
    {
        update_progress();
        return;
    }


    // --------------------------------------------------------
    // Build Marlin command
    // --------------------------------------------------------

    if (!build_marlin_command(
            gcode,
            current_command,
            sizeof(current_command)))
    {
        sender_state =
            SENDER_ERROR;

        set_status(
            "Command too long"
        );

        return;
    }


    update_command();
    update_progress();


    // --------------------------------------------------------
    // Send
    // --------------------------------------------------------

    send_command(
        current_command
    );


    // --------------------------------------------------------
    // Wait for Marlin
    // --------------------------------------------------------

    if (!wait_for_ok(5000))
    {
        sender_state =
            SENDER_ERROR;

        set_status(
            "Marlin timeout/resend"
        );

        Serial.println(
            F("Marlin did not return OK")
        );

        return;
    }


    // --------------------------------------------------------
    // Next Marlin line number
    // --------------------------------------------------------

    marlin_line_number++;
}


// ============================================================
// BUTTON CALLBACK
// ============================================================

static void button_event(
    lv_event_t *e)
{
    if (lv_event_get_code(e) !=
        LV_EVENT_CLICKED)
    {
        return;
    }

    lv_obj_t *button =
        lv_event_get_target_obj(e);

    if (button == start_button)
    {
        start_sender();
    }
    else if (button == pause_button)
    {
        pause_sender();
    }
    else if (button == stop_button)
    {
        stop_sender();
    }
}


// ============================================================
// SCAN SD CARD
// ============================================================



static void populate_file_list()
{
    String options;

    FsFile root =
        sd.open("/");

    if (!root)
    {
        set_status(
            "Cannot open SD root"
        );

        return;
    }

    FsFile entry;

    while (entry =
           root.openNextFile())
    {
        if (!entry.isDirectory())
        {
            char name[128];

            entry.getName(
                name,
                sizeof(name)
            );

            // Only show likely G-code files.

            String filename(name);

            filename.toLowerCase();

            if (filename.endsWith(".gcode") ||
                filename.endsWith(".g") ||
                filename.endsWith(".gc") ||
                filename.endsWith(".nc"))
            {
                if (options.length())
                    options += '\n';

                options += name;
            }
        }

        entry.close();
    }

    root.close();


    if (options.length() == 0)
    {
        lv_dropdown_set_options(
            file_dropdown,
            "No G-code files"
        );

        set_status(
            "No G-code files"
        );

        return;
    }

    lv_dropdown_set_options(
        file_dropdown,
        options.c_str()
    );
}


// ============================================================
// CREATE BUTTON
// ============================================================

static lv_obj_t *make_button(
    lv_obj_t *parent,
    const char *text,
    int x,
    int y)
{
    lv_obj_t *button =
        lv_button_create(parent);

    lv_obj_set_size(
        button,
        85,
        45
    );

    lv_obj_set_pos(
        button,
        x,
        y
    );

    lv_obj_t *label =
        lv_label_create(button);

    lv_label_set_text(
        label,
        text
    );

    lv_obj_center(label);

    lv_obj_add_event_cb(
        button,
        button_event,
        LV_EVENT_CLICKED,
        NULL
    );

    return button;
}


// ============================================================
// CREATE UI
// ============================================================

static void create_ui()
{
    lv_obj_t *screen =
        lv_screen_active();


    // --------------------------------------------------------
    // Title
    // --------------------------------------------------------

    lv_obj_t *title =
        lv_label_create(screen);

    lv_label_set_text(
        title,
        "G-Code Sender"
    );

    lv_obj_align(
        title,
        LV_ALIGN_TOP_MID,
        0,
        5
    );


    // --------------------------------------------------------
    // File dropdown
    // --------------------------------------------------------

    file_dropdown =
        lv_dropdown_create(screen);

    lv_obj_set_width(
        file_dropdown,
        270
    );

    lv_obj_set_pos(
        file_dropdown,
        25,
        30
    );


    // --------------------------------------------------------
    // Buttons
    // --------------------------------------------------------

    start_button =
        make_button(
            screen,
            "START",
            15,
            80
        );

    pause_button =
        make_button(
            screen,
            "PAUSE",
            117,
            80
        );

    stop_button =
        make_button(
            screen,
            "STOP",
            219,
            80
        );


    // --------------------------------------------------------
    // Progress bar
    // --------------------------------------------------------

    progress_bar =
        lv_bar_create(screen);

    lv_obj_set_size(
        progress_bar,
        290,
        20
    );

    lv_obj_set_pos(
        progress_bar,
        15,
        140
    );

    lv_bar_set_range(
        progress_bar,
        0,
        100
    );

    lv_bar_set_value(
        progress_bar,
        0,
        LV_ANIM_OFF
    );


    // --------------------------------------------------------
    // Line counter
    // --------------------------------------------------------

    line_label =
        lv_label_create(screen);

    lv_label_set_text(
        line_label,
        "Line 0 / 0"
    );

    lv_obj_set_pos(
        line_label,
        15,
        165
    );


    // --------------------------------------------------------
    // Current command
    // --------------------------------------------------------

    command_label =
        lv_label_create(screen);

    lv_label_set_text(
        command_label,
        "No command"
    );

    lv_obj_set_width(
        command_label,
        290
    );

    lv_label_set_long_mode(
        command_label,
        LV_LABEL_LONG_CLIP
    );

    lv_obj_set_pos(
        command_label,
        15,
        185
    );


    // --------------------------------------------------------
    // Status
    // --------------------------------------------------------

    status_label =
        lv_label_create(screen);

    lv_label_set_text(
        status_label,
        "Initializing..."
    );

    lv_obj_align(
        status_label,
        LV_ALIGN_BOTTOM_MID,
        0,
        -5
    );
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println(
        F("ESP32 Marlin G-Code Sender")
    );


    // --------------------------------------------------------
    // Marlin serial
    // --------------------------------------------------------

    MarlinSerial.begin(
        MARLIN_BAUD,
        SERIAL_8N1,
        16,       // RX
        17        // TX
    );


    // --------------------------------------------------------
    // LCD
    // --------------------------------------------------------

    lcd.init();

    lcd.setRotation(1);

    lcd.fillScreen(TFT_BLACK);


    // --------------------------------------------------------
    // LVGL
    // --------------------------------------------------------

    lv_init();

    lv_tick_set_cb(
        []() -> uint32_t
        {
            return millis();
        }
    );


    display =
        lv_display_create(
            LCD_W,
            LCD_H
        );

    lv_display_set_buffers(
        display,
        draw_buf,
        NULL,
        sizeof(draw_buf),
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    lv_display_set_flush_cb(
        display,
        display_flush
    );


    // --------------------------------------------------------
    // Touch
    // --------------------------------------------------------

    touch_indev =
        lv_indev_create();

    lv_indev_set_type(
        touch_indev,
        LV_INDEV_TYPE_POINTER
    );

    lv_indev_set_read_cb(
        touch_indev,
        touch_read
    );


    // --------------------------------------------------------
    // UI
    // --------------------------------------------------------

    create_ui();


    // --------------------------------------------------------
    // SD
    // --------------------------------------------------------

    if (initialize_sd())
    {
        populate_file_list();

        set_status(
            "Ready"
        );
    }
    else
    {
        set_status(
            "SD initialization failed"
        );
    }
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
    lv_timer_handler();

    process_gcode();

    delay(2);
}

#endif // 0