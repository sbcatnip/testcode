#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "http.h"  // httpebble
#include "config.h"  // contains all the #defines value
	

// Forward declarations
void refresh();
	
PBL_APP_INFO(MY_UUID,
             APP_NAME, APP_INC,
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_LOGO,
             APP_INFO_STANDARD_APP);

// Single Window system, 4 lines, 1 right nav bar bmp layer
Window window;
TextLayer text_layer1; 
TextLayer text_layer2;
TextLayer text_layer3;
TextLayer text_layer4;
ActionBarLayer action_bar_layer;

// Define the bmp available
HeapBitmap refresh_heap_bitmap;
HeapBitmap here_heap_bitmap;
HeapBitmap cancel_here_heap_bitmap;

// Global variables 
char global_page_id[] = "XXXXXXXXXXXXXXXXXXXXXXX";
int global_request_id = 0;
char global_layer1[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
char global_layer2[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
char global_layer3[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
char global_layer4[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
int global_timer_count = 0;
int global_timer_period = 30;
int global_refresh_invocation_count = 0;

// Define the Vibration Patterns
// 1 - SOS
// 2 - 5 short bursts
// 3 - 5 long bursts
// 4 - double pulse
const uint32_t const vibs1_segments[] = {100, 50, 100, 50, 100, 150, 300, 50, 300, 50, 300, 150, 100, 50, 100, 50, 100};
const uint32_t const vibs2_segments[] = {50, 50, 50, 50, 50, 50, 50, 50, 50};
const uint32_t const vibs3_segments[] = {350, 50, 350, 50, 350, 50, 350, 50, 350};

VibePattern pat1 = {
.durations = vibs1_segments,
.num_segments = ARRAY_LENGTH(vibs1_segments),
};

VibePattern pat2 = {
.durations = vibs2_segments,
.num_segments = ARRAY_LENGTH(vibs2_segments),
};

VibePattern pat3 = {
.durations = vibs3_segments,
.num_segments = ARRAY_LENGTH(vibs3_segments),
};
// End Vibration Patterns 

// Sets the font value of the layer based on string length 
// with textlayer window size over 38 pixels
// we can fit 
// 10 A with FONT SIZE 24
// 24 A with FONT SIZE 18
// 30 A with FONT SIZE 14
void setTextLayerValue (
  TextLayer* text_layer,
  const char *const global_layer) {

  const int global_layer_len = strlen (global_layer);
  if (global_layer_len < 11) {
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));	
  }
  else if (global_layer_len < 25) {
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));	
  }
  else {
	text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));  
  }
	
  text_layer_set_text(text_layer, global_layer);
}

// sets the value of text layers from dictionary, called on success http
void setTextLayerValueFromDict (
  DictionaryIterator* received,
  const int field_name,
  TextLayer* text_layer,
  char* global_layer) {
	
  Tuple* tuple = dict_find(received, field_name);
  if (tuple) {
	strcpy(global_layer, tuple->value->cstring);
	setTextLayerValue (text_layer, global_layer);
  }
  else {
	setTextLayerValue(text_layer, global_layer);
  }
}

/* Handle HTTP success response
*/
void handle_http_success(int32_t request_id, int http_status, DictionaryIterator* received, void* context) {
// checks if the request is from my app, if not, ignore the response
  if (request_id != HTTP_COOKIE) {
    return;
  }
	
// update the global_timer_period to what's specified on the server side.
  {
  	Tuple* poll_period_tuple = dict_find(received, FIELD_NAME_POLL_PERIOD);
  	if (poll_period_tuple){ 
 	  	global_timer_period = poll_period_tuple->value->int16;	
		
		// set hard limit in case poll time is too low (5 sec) or too high (300 sec - 5 min)
		if (global_timer_period < 5) {
		  global_timer_period = 5;
		} 
		if (global_timer_period > 300) { 
		  global_timer_period = 300;
		}
	}  
  }	

/*	
	Set each text layers with text layer values
*/
  setTextLayerValueFromDict (
	received, 
	FIELD_NAME_TEXT_LAYER1, 
	&text_layer1,  
	global_layer1
  );

  setTextLayerValueFromDict (
	received, 
	FIELD_NAME_TEXT_LAYER2, 
	&text_layer2,  
	global_layer2
  );
	  	
  setTextLayerValueFromDict (
	received, 
	FIELD_NAME_TEXT_LAYER3, 
	&text_layer3,  
	global_layer3
  );

  setTextLayerValueFromDict (
	received, 
	FIELD_NAME_TEXT_LAYER4, 
	&text_layer4,  
	global_layer4
  );
	

// Checks for vibrate flag and vibrate according to value
// 1 - SOS
// 2 - 5 short bursts
// 3 - 5 long bursts
// 4 - double pulse
  {		
  	Tuple* page_vibrate_flag_tuple = dict_find(received, FIELD_NAME_VIBRATE_FLAG);
  	if (page_vibrate_flag_tuple->value->int8 == 1) {
	vibes_enqueue_custom_pattern(pat1);
    }	
	else if (page_vibrate_flag_tuple->value->int8 == 2) {
	vibes_enqueue_custom_pattern(pat2);
    }			
	else if (page_vibrate_flag_tuple->value->int8 == 3) {
	vibes_enqueue_custom_pattern(pat3);
    }	
	else if (page_vibrate_flag_tuple->value->int8 == 4) {
	  vibes_double_pulse();
    }	
  }		
	
  {	
// saved the page_id so we can set next location to here
  	Tuple* page_id_tuple = dict_find(received, FIELD_NAME_PAGE_ID);
	if (page_id_tuple) {
  	  strcpy(global_page_id, page_id_tuple->value->cstring);
    }	
  }

// if server has more data then request again	
  {		
  	Tuple* more_flag_tuple = dict_find(received, FIELD_NAME_HEADER_MORE_FLAG);
  	if (more_flag_tuple) {
	  if (more_flag_tuple->value->int8 == 1) {
	  	refresh();
      }	
	}  
  }		
}

// HTTP Error Reporting
void httpebble_error(int error_code) {
	
  APP_LOG (APP_LOG_LEVEL_INFO, "httpebble_error (). error_code: %d", error_code);	// DEBUG	
  
  static char error_message[] = "UNKNOWN_HTTP_ERRROR_CODE_GENERATED";

  switch (error_code) {
	case HTTP_SEND_TIMEOUT:
	strcpy(error_message, "HTTP_SEND_TIMEOUT");
	break;
	case HTTP_SEND_REJECTED:
	strcpy(error_message, "HTTP_SEND_REJECTED");
	break;
	case HTTP_NOT_CONNECTED:
	strcpy(error_message, "HTTP_NOT_CONNECTED");
	break;
	case HTTP_BRIDGE_NOT_RUNNING:
	strcpy(error_message, "HTTP_BRIDGE_NOT_RUNNING");
	break;
	case HTTP_INVALID_ARGS:
	strcpy(error_message, "HTTP_INVALID_ARGS");
	break;
	case HTTP_BUSY:
	strcpy(error_message, "HTTP_BUSY");
	break;
	case HTTP_BUFFER_OVERFLOW:
	strcpy(error_message, "HTTP_BUFFER_OVERFLOW");
	break;
	case HTTP_ALREADY_RELEASED:
	strcpy(error_message, "HTTP_ALREADY_RELEASED");
	break;
	case HTTP_CALLBACK_ALREADY_REGISTERED:
	strcpy(error_message, "HTTP_CALLBACK_ALREADY_REGISTERED");
	break;
	case HTTP_CALLBACK_NOT_REGISTERED:
	strcpy(error_message, "HTTP_CALLBACK_NOT_REGISTERED");
	break;
	case HTTP_NOT_ENOUGH_STORAGE:
	strcpy(error_message, "HTTP_NOT_ENOUGH_STORAGE");
	break;
	case HTTP_INVALID_DICT_ARGS:
	strcpy(error_message, "HTTP_INVALID_DICT_ARGS");
	break;
	case HTTP_INTERNAL_INCONSISTENCY:
	strcpy(error_message, "HTTP_INTERNAL_INCONSISTENCY");
	break;
	case HTTP_INVALID_BRIDGE_RESPONSE:
	strcpy(error_message, "HTTP_INVALID_BRIDGE_RESPONSE");
	break;
	default: {
	  strcpy(error_message, "HTTP_ERROR_UNKNOWN");
    } 
}

// writes error message and friendly retry with smaller fonts
  setTextLayerValue(&text_layer2, "HTTP ERROR");
  setTextLayerValue(&text_layer3, error_message);
  setTextLayerValue(&text_layer4, "Retrying soon...");

// zero out my global variables	when an error occurs
  strcpy(global_page_id, "");	
  strcpy(global_layer1, "");	
  strcpy(global_layer2, "");
  strcpy(global_layer3, "");
  strcpy(global_layer4, "");
	
// reset request id to get fresh data after an error
  global_request_id = 0;	
}


// Handle HTTP failure
void handle_http_failure(int32_t request_id, int http_status, void* context) {
  httpebble_error(http_status >= 1000 ? http_status - 1000 : http_status);
}

// refresh content from server
void refresh() {

  APP_LOG (APP_LOG_LEVEL_INFO, "refresh ()");	// DEBUG	
	
  global_refresh_invocation_count++;	
  DictionaryIterator* dict;
  HTTPResult result = http_out_get(SERVER_URL, HTTP_COOKIE, &dict);	
  if (result != HTTP_OK) {
	httpebble_error(result);
    return;
  }
// builds the request - standard request string
  global_request_id++;	
  dict_write_cstring(dict, FIELD_NAME_HEADER_VERSION, HTTP_MESSAGE_VERSION);
  dict_write_int32(dict, FIELD_NAME_HEADER_REQUEST_ID, global_request_id);
  dict_write_cstring(dict, FIELD_NAME_HEADER_OPERATION_NAME, OPERATION_NAME_LOAD);	

  if (global_refresh_invocation_count % 20 == 0 || global_refresh_invocation_count == 1) {
	dict_write_cstring(dict, FIELD_NAME_HEADER_INFO_STRING, INFO_STRING);		
  }
	
  result = http_out_send();
  if (result != HTTP_OK) {
	httpebble_error(result);
    return;
  }
}

// check into next location
void here() {

//checks if there's no next location, if there's no next location do nothing
  if (strlen (global_page_id) == 0) {
	return;
  }	
	
//build the check in request
  DictionaryIterator* dict;
  HTTPResult result = http_out_get(SERVER_URL, HTTP_COOKIE, &dict);	
  if (result != HTTP_OK) {
	httpebble_error(result);
    return;
  }
// builds the request - check into next location string
  global_request_id++;
  dict_write_cstring(dict, FIELD_NAME_HEADER_VERSION, HTTP_MESSAGE_VERSION);
  dict_write_int32(dict, FIELD_NAME_HEADER_REQUEST_ID, global_request_id);	
  dict_write_cstring(dict, FIELD_NAME_HEADER_OPERATION_NAME, OPERATION_NAME_HERE);	
  dict_write_cstring(dict, FIELD_NAME_PAGE_ID, global_page_id);
	  
  result = http_out_send();
  if (result != HTTP_OK) {
	httpebble_error(result);
    return;
  }
	
// Get the latest data after the checkin
  refresh();
}

// clear next location
void cancel_here() {

//build the check in request
  DictionaryIterator* dict;
  HTTPResult result = http_out_get(SERVER_URL, HTTP_COOKIE, &dict);	
  if (result != HTTP_OK) {
	httpebble_error(result);
    return;
  }
// builds the request - check into next location string
// Insert Clear the board logic here
  global_request_id++;
  dict_write_cstring(dict, FIELD_NAME_HEADER_VERSION, HTTP_MESSAGE_VERSION);
  dict_write_int32(dict, FIELD_NAME_HEADER_REQUEST_ID, global_request_id);	
  dict_write_cstring(dict, FIELD_NAME_HEADER_OPERATION_NAME, OPERATION_NAME_HERE);	
  dict_write_cstring(dict, FIELD_NAME_PAGE_ID, global_page_id);
	  
  result = http_out_send();
  if (result != HTTP_OK) {
	httpebble_error(result);
    return;
  }	
	
// Get the latest data after the checkin
  refresh();	
}

// select button click handler
void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  refresh();
}

// down button click handler
void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  here();
}

// up button click handler
void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  cancel_here();
}	

/* Define button availability
Select button - Refreshes the Page status
Down button - Checks into Next Location
Up button - clears the board
*/
void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;
  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;
  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
}

// Application Initialization
void handle_init(AppContextRef ctx) {
	
  APP_LOG (APP_LOG_LEVEL_INFO, "handle_init ()");	// DEBUG
	
  (void)ctx;
  http_set_app_id(HTTP_APP_ID);

// sets up APP_RESOURCES for side nav bar
  resource_init_current_app(&RESOURCE_ID);


// zero out my global variables	
  strcpy(global_page_id, "");
  strcpy(global_layer1, "");	
  strcpy(global_layer2, "");
  strcpy(global_layer3, "");
  strcpy(global_layer4, "");
	
// register callback handlers functions on success and failure	 
  http_register_callbacks((HTTPCallbacks) {
  	.success = handle_http_success,
  	.failure = handle_http_failure
  }, NULL);
	
  // setups up the window
  window_init(&window, "Main Window");
	
//  Dont use this anymore as we use actionbar for click provider	
//  possible server side toggle to use actionbar or not?	
//  window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);	

  // Load bitmaps and set action bar icons
  heap_bitmap_init(&refresh_heap_bitmap, RESOURCE_ID_IMAGE_REFRESH);
  heap_bitmap_init(&here_heap_bitmap, RESOURCE_ID_IMAGE_HERE);
  heap_bitmap_init(&cancel_here_heap_bitmap, RESOURCE_ID_IMAGE_CANCEL_HERE);	

  action_bar_layer_init(&action_bar_layer);
  action_bar_layer_set_click_config_provider(&action_bar_layer, (ClickConfigProvider) click_config_provider);	
  action_bar_layer_set_icon(&action_bar_layer, BUTTON_ID_SELECT, &refresh_heap_bitmap.bmp);
  action_bar_layer_set_icon(&action_bar_layer, BUTTON_ID_DOWN, &here_heap_bitmap.bmp);
  action_bar_layer_set_icon(&action_bar_layer, BUTTON_ID_UP, &cancel_here_heap_bitmap.bmp);
	
  window_stack_push(&window, true /* Animated */);

// paints the action bar layer	
  action_bar_layer_add_to_window(&action_bar_layer, &window);	
	
// Sets up the 4 text layers with some default content.	
// Pebble has width of 144 pixels and height of 168 pixels
// top status bar (by default shows time) is 16 pixels
// action bar on the right is 20 pixels wide	
// this leaves us with with of 124 pixels from the left
// by height of 152 pixel divided into 4 equal layers (4x 38 pixels)	
// we alternate black/clear text/background color for readability/contrast	
  text_layer_init(&text_layer1, GRect(0, 0, 124, 38));
  text_layer_set_text_color(&text_layer1, GColorBlack);
  text_layer_set_background_color(&text_layer1, GColorClear);
  text_layer_set_font(&text_layer1, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&text_layer1, GTextAlignmentCenter);
  layer_add_child(&window.layer, &text_layer1.layer);
	
  text_layer_init(&text_layer2, GRect(0, 38, 124, 38));
  text_layer_set_text_color(&text_layer2, GColorClear);
  text_layer_set_background_color(&text_layer2, GColorBlack);
  text_layer_set_font(&text_layer2, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&text_layer2, GTextAlignmentCenter);
  text_layer_set_text(&text_layer2, "DoctorMeow");		
  layer_add_child(&window.layer, &text_layer2.layer);

  text_layer_init(&text_layer3, GRect(0, 76, 124, 38));
  text_layer_set_text_color(&text_layer3, GColorBlack);
  text_layer_set_background_color(&text_layer3, GColorClear);
  text_layer_set_font(&text_layer3, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&text_layer3, GTextAlignmentCenter);
  text_layer_set_text(&text_layer3, "Loading...");
  layer_add_child(&window.layer, &text_layer3.layer);	

  text_layer_init(&text_layer4, GRect(0, 114, 124, 38));
  text_layer_set_text_color(&text_layer4, GColorClear);
  text_layer_set_background_color(&text_layer4, GColorBlack);
  text_layer_set_font(&text_layer4, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&text_layer4, GTextAlignmentCenter);		
  layer_add_child(&window.layer, &text_layer4.layer);	

  // Do the initial refresh to get data
  refresh();	
}

// Deinit and free up the memory for bitmaps and window
void handle_deinit(AppContextRef ctx)
{
  heap_bitmap_deinit(&refresh_heap_bitmap);
  heap_bitmap_deinit(&here_heap_bitmap);
  heap_bitmap_deinit(&cancel_here_heap_bitmap);	
  window_deinit(&window);	
}


// Second tick handler - increments my second counter until I hit server defined variable
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t)
{
	global_timer_count++;
	if (global_timer_count >= global_timer_period) {
	  global_timer_count = 0;
	  refresh();
	};
}	


// main program
void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
	.deinit_handler = &handle_deinit,
	.tick_info = {
	  .tick_handler = &handle_second_tick,
	  .tick_units = SECOND_UNIT
	},
	.messaging_info = {
		.buffer_sizes = {
		  .inbound = 124,
		  .outbound = 256,
		}	
	}
  };
  app_event_loop(params, &handlers);
}
