#define CS_ALWAYS_LOW

#define TFT_DC 7
#define TFT_RST 8
#define SCR_WD 240
#define SCR_HT 240  // 320 - to allow access to full 240x320 frame buffer
#include <Adafruit_GFX.h>
#include <Arduino_ST7789_Fast.h>
#include <ClickEncoder.h>
#include <SPI.h>
#include <TimerOne.h>

Arduino_ST7789 lcd = Arduino_ST7789( TFT_DC, TFT_RST );

ClickEncoder* encoder;
int16_t encoder_value = 0;
int16_t old_encoder_value = 0;
bool encoder_pos_changed = false;

void reboot( );

struct MenuItem
{
    const char* text;
    void ( *fptr )( );
};

const MenuItem menu_items[]
    = {{"Temperature"}, {"Humidity"},    {"Both"},  {"Battery"}, {"Backlight"},
       {"Contrast"},    {"EEPROM dump"}, {"Graph"}, {"Help"},    {"Reboot", reboot}};

int8_t menu_mode = -1;
bool menu_item_changed = false;

int16_t tmp_int16;
uint16_t tmp_uint16;

void
timerIsr( )
{
    encoder->service( );
}

void
setup( )
{
    Serial.begin( 9600 );
    encoder = new ClickEncoder( A1, A2, A3 );

    lcd.init( SCR_WD, SCR_HT );
    lcd.fillScreen( BLACK );
    lcd.setTextSize( 3 );
    lcd.setTextColor( WHITE );

    Timer1.initialize( 1000 );
    Timer1.attachInterrupt( timerIsr );

    handle_menu( );
}

bool
check_encoder_changed( )
{
    const auto prev_changed = encoder_pos_changed;
    encoder_pos_changed = false;
    return prev_changed;
}

bool
check_menu_item_changed( )
{
    const auto prev_changed = menu_item_changed;
    menu_item_changed = false;
    return prev_changed;
}

bool
check_button_clicked( )
{
    ClickEncoder::Button b = encoder->getButton( );
    if ( b != ClickEncoder::Open )
    {
        return b == ClickEncoder::Clicked;
    }
    return false;
}

void
to_menu_item( int8_t mode )
{
    menu_mode = mode;
    old_encoder_value = encoder_value;
    encoder_value = 0;
    encoder_pos_changed = true;
    menu_item_changed = true;
}

void
to_main_menu( )
{
    menu_mode = -1;
    encoder_value = old_encoder_value;
    encoder_pos_changed = true;
    menu_item_changed = true;
}

void ( *doReset )( void ) = 0;
void
reboot( )
{
    static uint16_t text_height;
    if ( check_menu_item_changed( ) )
    {
        lcd.fillScreen( BLACK );
        lcd.setTextColor( RED );
        const auto reboot_str = "REBOOT";
        uint16_t reboot_str_width;
        lcd.getTextBounds( reboot_str, 0, 0, &tmp_int16, &tmp_int16, &reboot_str_width,
                           &text_height );
        lcd.setCursor( ( SCR_WD - reboot_str_width ) / 2, 0 );
        lcd.println( reboot_str );
        lcd.drawRect( 0, 100, SCR_WD, text_height + 2, BLUE );
    }
    const bool reboot = ( encoder_value / 2 ) & 1;
    if ( check_encoder_changed( ) )
    {
        lcd.fillRect( 1, 101, SCR_WD - 2, text_height, BLACK );
        lcd.fillRect( reboot ? ( ( SCR_WD - 2 ) / 2 + 1 ) : 1, 101, ( SCR_WD - 2 ) / 2, text_height,
                      BLUE );
        uint16_t str_width;
        lcd.setTextColor( WHITE );
        const auto no_str = "NO";
        lcd.getTextBounds( no_str, 0, 0, &tmp_int16, &tmp_int16, &str_width, &tmp_uint16 );
        lcd.setCursor( ( ( SCR_WD - 2 ) / 2 - str_width ) / 2, 101 );
        lcd.print( no_str );
        const auto yes_str = "YES";
        lcd.getTextBounds( yes_str, 0, 0, &tmp_int16, &tmp_int16, &str_width, &tmp_uint16 );
        lcd.setCursor( ( ( SCR_WD - 2 ) / 2 - str_width ) / 2 + ( ( SCR_WD - 2 ) / 2 + 1 ), 101 );
        lcd.print( yes_str );
    }
    if ( check_button_clicked( ) )
    {
        if ( reboot )
        {
            lcd.clearScreen( );
            doReset( );
        }
        else
        {
            to_main_menu( );
        }
    }
}

void
handle_main_menu( )
{
    static const uint8_t menu_items_count = sizeof( menu_items ) / sizeof( menu_items[ 0 ] );
    static const uint8_t menu_items_on_screen = 6;

    static int16_t menu_line = -1;
    static uint8_t menu_start = 0;

    const auto prev_menu_line = menu_line;
    menu_line = encoder_value / 2;
    if ( menu_line >= menu_items_count )
    {
        menu_line = menu_items_count - 1;
        encoder_value = menu_line * 2;
    }
    else if ( menu_line < 0 )
    {
        menu_line = 0;
        encoder_value = 0;
    }

    if ( check_button_clicked( ) )
    {
        to_menu_item( menu_line );
        return;
    }

    if ( check_menu_item_changed( ) )
    {
    }
    else if ( menu_line == prev_menu_line )
    {
        return;
    }

    if ( menu_line >= menu_start + menu_items_on_screen )
    {
        menu_start = menu_line - menu_items_on_screen + 1;
    }
    else if ( menu_line < menu_start )
    {
        menu_start = menu_line;
    }

    lcd.fillScreen( BLACK );

    static const char* main_menu_str = "MAIN MENU";
    uint16_t main_menu_str_left;
    lcd.getTextBounds( main_menu_str, 0, 0, &tmp_int16, &tmp_int16, &main_menu_str_left,
                       &tmp_uint16 );
    main_menu_str_left = ( SCR_WD - main_menu_str_left ) / 2;
    lcd.setCursor( main_menu_str_left, 0 );
    lcd.setTextSize( 3 );
    lcd.setTextColor( GREEN );
    lcd.println( main_menu_str );
    const auto cursor_y = lcd.getCursorY( );
    lcd.drawLine( 0, cursor_y + 10, SCR_WD, cursor_y + 10, MAGENTA );
    lcd.setCursor( 0, cursor_y + 20 );

    // draw slider
    static const int16_t slider_height = 80;
    lcd.drawLine( SCR_WD - 10, lcd.getCursorY( ), SCR_WD - 10, SCR_HT - 10, YELLOW );
    const auto slider_offset_step
        = ( SCR_HT - 10 - lcd.getCursorY( ) - slider_height ) / ( menu_items_count - 1 ) + 1;
    const auto slider_offset = slider_offset_step * menu_line;
    lcd.fillRect( SCR_WD - 13, lcd.getCursorY( ) + slider_offset, 7, slider_height, BLUE );

    for ( uint8_t i = 0; i < menu_items_on_screen; ++i )
    {
        const auto item = i + menu_start;
        if ( item == menu_line )
        {
            lcd.setTextColor( YELLOW, BLUE );
        }
        else
        {
            lcd.setTextColor( WHITE );
        }
        lcd.println( menu_items[ i + menu_start ].text );
        lcd.setCursor( 0, lcd.getCursorY( ) + 10 );
    }
}

void
handle_menu( )
{
    if ( menu_mode == -1 )
    {
        handle_main_menu( );
    }
    else
    {
        menu_items[ menu_mode ].fptr( );
    }
}

bool
update_encoder( )
{
    auto prev_val = encoder_value;
    encoder_value += encoder->getValue( );
    return prev_val != encoder_value;
}

void
loop( )
{
    static unsigned long ms_interval = 1000;
    static unsigned long previous_ms = 0;
    if ( ( millis( ) - previous_ms ) >= ms_interval )
    {
        // TODO: Implement sensor data update
        previous_ms = millis( );
    }

    encoder_pos_changed |= update_encoder( );

    handle_menu( );
}
