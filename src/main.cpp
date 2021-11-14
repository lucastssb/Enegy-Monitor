
#include <lvgl.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
lgfx::Panel_ILI9488     _panel_instance;
lgfx::Bus_SPI       _bus_instance;   // SPI
lgfx::Light_PWM     _light_instance;
lgfx::Touch_XPT2046     _touch_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();    
      cfg.spi_host = VSPI_HOST;     // (VSPI_HOST or HSPI_HOST)
      cfg.spi_mode = 0;             // SPI (0 ~ 3)
      cfg.freq_write = 40000000;    //  (80MHz, 80MHz)
      cfg.freq_read  = 16000000;    // SPI
      cfg.spi_3wire  = false;        // MOSI true
      cfg.use_lock   = true;        // SPI lock
      cfg.dma_channel = 1;          // Set the DMA channel (1 or 2. 0=disable)
      cfg.pin_sclk = 18;            // SPI SCLK
      cfg.pin_mosi = 23;            // SPI MOSI
      cfg.pin_miso = 19;            // SPI MISO (-1 = disable)
      cfg.pin_dc   = 2;            // SPI D/C  (-1 = disable)
      _bus_instance.config(cfg);  
      _panel_instance.setBus(&_bus_instance); 
    }

    {
      auto cfg = _panel_instance.config();    
      cfg.pin_cs           =    5;  // (-1 = disable)
      cfg.pin_rst          =    4;  // (-1 = disable)
      cfg.pin_busy         =    -1;  // BUSY (-1 = disable)
      cfg.memory_width     =   320;  
      cfg.memory_height    =   480;  
      cfg.panel_width      =   320;  
      cfg.panel_height     =   480;  
      cfg.offset_x         =     0;  
      cfg.offset_y         =     0;  
      cfg.offset_rotation  =     0;  
      cfg.dummy_read_pixel =     8;  
      cfg.dummy_read_bits  =     1;  
      cfg.readable         =  true;  
      cfg.invert           = false;  
      cfg.rgb_order        = false;  
      cfg.dlen_16bit       = false;  
      cfg.bus_shared       =  true;  

      _panel_instance.config(cfg);
    }
    
    { 
      auto cfg = _light_instance.config();   

      cfg.pin_bl = 22;              
      cfg.invert = false;           
      cfg.freq   = 44100;          
      cfg.pwm_channel = 7;          

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  
    }

    { 
      auto cfg = _touch_instance.config();
      cfg.x_min      = 0;    
      cfg.x_max      = 319;  
      cfg.y_min      = 0;    
      cfg.y_max      = 479; 
      cfg.pin_int    = -1;   
      cfg.bus_shared = true; 
      cfg.offset_rotation = 0;
      cfg.spi_host = VSPI_HOST;// (HSPI_HOST or VSPI_HOST)
      cfg.freq = 1000000;     // SPI
      cfg.pin_sclk = 18;     // SCLK
      cfg.pin_mosi = 23;     // MOSI
      cfg.pin_miso = 19;     // MISO
      cfg.pin_cs   = 15;     //   CS
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);  
    }
    setPanel(&_panel_instance); 
  }
};

LGFX tft;

/*Change to your screen resolution*/
static const uint32_t screenWidth  = 480;
static const uint32_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * 10 ];

/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
   uint32_t w = ( area->x2 - area->x1 + 1 );
   uint32_t h = ( area->y2 - area->y1 + 1 );

   tft.startWrite();
   tft.setAddrWindow( area->x1, area->y1, w, h );
   //tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
   tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
   tft.endWrite();

   lv_disp_flush_ready( disp );
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
   uint16_t touchX, touchY;
   bool touched = tft.getTouch( &touchX, &touchY);
   if( !touched )
   {
      data->state = LV_INDEV_STATE_REL;
   }
   else
   {
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = touchX;
      data->point.y = touchY;

      Serial.print( "Data x " );
      Serial.println( touchX );

      Serial.print( "Data y " );
      Serial.println( touchY );
   }
}

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;

        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

/**
 * Create a button with a label and react on click event.
 */
void lv_example_get_started_1(void)
{
    lv_obj_t * btn = lv_btn_create(lv_scr_act());     /*Add a button the current screen*/
    lv_obj_set_size(btn, 120, 50);                          /*Set its size*/
    lv_obj_align(btn, LV_ALIGN_CENTER, 0,0);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * label = lv_label_create(btn);          /*Add a label to the button*/
    lv_label_set_text(label, "Button");                     /*Set the labels text*/
    lv_obj_center(label);
}

void setup()
{
   Serial.begin(115200);

   tft.begin();        
   tft.setRotation(1);
   tft.setBrightness(255);
   uint16_t calData[] = { 239, 3926, 233, 265, 3856, 3896, 3714, 308};
   tft.setTouchCalibrate(calData);

   lv_init();
   lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * 10 );

   /*Initialize the display*/
   static lv_disp_drv_t disp_drv;
   lv_disp_drv_init(&disp_drv);

   /*Change the following line to your display resolution*/
   disp_drv.hor_res = screenWidth;
   disp_drv.ver_res = screenHeight;
   disp_drv.flush_cb = my_disp_flush;
   disp_drv.draw_buf = &draw_buf;
   lv_disp_drv_register(&disp_drv);

   /*Initialize the (dummy) input device driver*/
   static lv_indev_drv_t indev_drv;
   lv_indev_drv_init(&indev_drv);
   indev_drv.type = LV_INDEV_TYPE_POINTER;
   indev_drv.read_cb = my_touchpad_read;
   lv_indev_drv_register(&indev_drv);

   lv_example_get_started_1();
}

void loop()
{
   lv_timer_handler(); /* let the GUI do its work */
   delay( 5 );
}

