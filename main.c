
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "http.h"

/* If compiling this for iOS, set ANDROID to be false. */
#define ANDROID false

#if ANDROID
#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0x10, 0x34, 0xBF, 0xBE, 0x12, 0x98 }
#else
#define MY_UUID HTTP_UUID
#endif

#define HTTP_COOKIE 4887

#define ONMYWAY 9999  

PBL_APP_INFO(MY_UUID, "DoctorMeow", "DoctorMeow, Inc.", 1, 0, DEFAULT_MENU_ICON, APP_INFO_STANDARD_APP);


void handle_init(AppContextRef ctx);
void http_success(int32_t request_id, int http_status, DictionaryIterator* received, void* context);
void http_failure(int32_t request_id, int http_status, void* context);
void window_appear(Window *window);
void httpebble_error(int error_code);
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t);
void request_update();
void select_single_click_handler(ClickRecognizerRef recognizer, Window *window);
void click_config_provider(ClickConfig **config, Window *window);
void handle_deinit (AppContextRef ctx);

Window window;
TextLayer resource_layer;
TextLayer current_loc_layer;
TextLayer next_loc_layer;
TextLayer next_loctime_layer;

bool One_Window = true;

void pbl_main(void *params) {
PebbleAppHandlers handlers = {
.init_handler = &handle_init,
.deinit_handler = &handle_deinit,
.tick_info =
{
.tick_handler =&handle_minute_tick,
.tick_units = MINUTE_UNIT
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

void handle_deinit (AppContextRef ctx) {
window_deinit(&window);
}


void click_config_provider(ClickConfig **config, Window *window) {
(void)window;

config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;

// config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;

// config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
// config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

// config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
// config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}



void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
(void)recognizer;
(void)window;
vibes_double_pulse();
text_layer_set_text(¤t_loc_layer, "On My Way!");
DictionaryIterator* dict;
HTTPResult result = http_out_get("http://sbcatnip.dyndns.org/portal/pebble", ONMYWAY, &dict);
if (result != HTTP_OK) {
httpebble_error(result);
return;
}

result = http_out_send();
if (result != HTTP_OK) {
httpebble_error(result);
return;
}

}


void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t)
{
request_update();
}	


void request_update() {
http_register_callbacks((HTTPCallbacks) {
.success = http_success,
.failure = http_failure
}, NULL);

window_init(&window, "Handler Window1");
window_stack_push(&window, false);

window_set_window_handlers(&window, (WindowHandlers){
.appear = window_appear
});

text_layer_init(&resource_layer, GRect(0, 0, 144, 35));
text_layer_set_text_color(&resource_layer, GColorBlack);
text_layer_set_background_color(&resource_layer, GColorClear);
text_layer_set_font(&resource_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
text_layer_set_text_alignment(&resource_layer, GTextAlignmentCenter);
layer_add_child(&window.layer, &resource_layer.layer);

text_layer_init(¤t_loc_layer, GRect(0, 35, 144, 39));
text_layer_set_text_color(¤t_loc_layer, GColorClear);
text_layer_set_background_color(¤t_loc_layer, GColorBlack);
text_layer_set_font(¤t_loc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
text_layer_set_text_alignment(¤t_loc_layer, GTextAlignmentCenter);
layer_add_child(&window.layer, ¤t_loc_layer.layer);

text_layer_init(&next_loc_layer, GRect(0, 74, 144, 38));
text_layer_set_text_color(&next_loc_layer, GColorBlack);
text_layer_set_background_color(&next_loc_layer, GColorClear);
text_layer_set_font(&next_loc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
text_layer_set_text_alignment(&next_loc_layer, GTextAlignmentCenter);
layer_add_child(&window.layer, &next_loc_layer.layer);	


text_layer_init(&next_loctime_layer, GRect(0, 112, 144, 42));
text_layer_set_text_color(&next_loctime_layer, GColorBlack);
text_layer_set_background_color(&next_loctime_layer, GColorClear);
text_layer_set_font(&next_loctime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
text_layer_set_text_alignment(&next_loctime_layer, GTextAlignmentCenter);
layer_add_child(&window.layer, &next_loctime_layer.layer);

window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);

layer_mark_dirty(&window.layer);
}	

// custom vibration

// static const uint32_t const segments[] = {400, 100, 200, 100, 200, 100, 250, 100, 200, 300, 200, 100, 200};

// Play SOS
static const uint32_t const segments[] = {100, 50, 100, 50, 100, 150, 300, 50, 300, 50, 300, 150, 100, 50, 100, 50, 100};
VibePattern pat = {
.durations = segments,
.num_segments = ARRAY_LENGTH(segments),
};

void http_success(int32_t request_id, int http_status, DictionaryIterator* received, void* context) {
if (request_id != HTTP_COOKIE) {
return;
}

Tuple* tuple1 = dict_find(received, 0);
text_layer_set_text(&resource_layer, tuple1->value->cstring);

Tuple* tuple2 = dict_find(received, 1);
text_layer_set_text(¤t_loc_layer, tuple2->value->cstring);

Tuple* tuple3 = dict_find(received, 2);
text_layer_set_text(&next_loc_layer, tuple3->value->cstring);

Tuple* tuple4 = dict_find(received, 3);
text_layer_set_text(&next_loctime_layer, tuple4->value->cstring);

Tuple* tuple5 = dict_find(received, 4);
if (tuple5) {
vibes_enqueue_custom_pattern(pat);
}

}

void http_failure(int32_t request_id, int http_status, void* context) {
httpebble_error(http_status >= 1000 ? http_status - 1000 : http_status);
}

void window_appear(Window* window) {

DictionaryIterator* dict;
HTTPResult result = http_out_get("http://sbcatnip.dyndns.org/portal/pebble", HTTP_COOKIE, &dict);
if (result != HTTP_OK) {
httpebble_error(result);
return;
}

result = http_out_send();
if (result != HTTP_OK) {
httpebble_error(result);
return;
}
}

void handle_init(AppContextRef ctx) {
http_set_app_id(76782702);

http_register_callbacks((HTTPCallbacks) {
.success = http_success,
.failure = http_failure
}, NULL);

window_init(&window, "First Window");
window_stack_push(&window, true);
window_set_window_handlers(&window, (WindowHandlers){
.appear = window_appear
});

text_layer_init(&resource_layer, GRect(0, 30, 144, 26));
text_layer_set_text_color(&resource_layer, GColorBlack);
text_layer_set_background_color(&resource_layer, GColorClear);
text_layer_set_font(&resource_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
text_layer_set_text_alignment(&resource_layer, GTextAlignmentCenter);
layer_add_child(&window.layer, &resource_layer.layer);

text_layer_init(¤t_loc_layer, GRect(0, 80, 144, 26));
text_layer_set_text_color(¤t_loc_layer, GColorClear);
text_layer_set_background_color(¤t_loc_layer, GColorBlack);
text_layer_set_font(¤t_loc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
text_layer_set_text_alignment(¤t_loc_layer, GTextAlignmentCenter);
layer_add_child(&window.layer, ¤t_loc_layer.layer);

text_layer_init(&next_loc_layer, GRect(0, 74, 144, 38));
text_layer_set_text_color(&next_loc_layer, GColorBlack);
text_layer_set_background_color(&next_loc_layer, GColorClear);
text_layer_set_font(&next_loc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
text_layer_set_text_alignment(&next_loc_layer, GTextAlignmentCenter);
layer_add_child(&window.layer, &next_loc_layer.layer);


text_layer_init(&next_loctime_layer, GRect(0, 112, 144, 42));
text_layer_set_text_color(&next_loctime_layer, GColorBlack);
text_layer_set_background_color(&next_loctime_layer, GColorClear);
text_layer_set_font(&next_loctime_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
text_layer_set_text_alignment(&next_loctime_layer, GTextAlignmentCenter);
layer_add_child(&window.layer, &next_loctime_layer.layer);

// Attach our desired button functionality
window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);

}

void httpebble_error(int error_code) {

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

text_layer_set_font(¤t_loc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
text_layer_set_text(¤t_loc_layer, error_message);
}
