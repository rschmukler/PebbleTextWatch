#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "resource_ids.auto.h"

#include "num2words-en.h"

#define DEBUG 0
#define BUFFER_SIZE 44

#define MY_UUID { 0x51, 0xFA, 0x89, 0x87, 0xCF, 0x51, 0x4F, 0x9C, 0xA3, 0x0C, 0x22, 0xB0, 0x3D, 0xCD, 0x21, 0x20 }
PBL_APP_INFO(MY_UUID,
             "Text With Date and oh", "Ryan Schmukler",
             1, 0,
             DEFAULT_MENU_ICON,
#if DEBUG
             APP_INFO_STANDARD_APP
#else
			 APP_INFO_WATCH_FACE
#endif
);

Window window;

typedef struct {
	TextLayer currentLayer;
	TextLayer nextLayer;	
	PropertyAnimation currentAnimation;
	PropertyAnimation nextAnimation;
} Line;

Line line1;
Line line2;
Line line3;

PblTm t;

static char line1Str[2][BUFFER_SIZE];
static char line2Str[2][BUFFER_SIZE];
static char line3Str[2][BUFFER_SIZE];

TextLayer topDayLayer;
TextLayer bottomDayLayer;

static const char* const WEEKDAYS[] = {
"sunday",
"monday",
"tuesday",
"wednesday",
"thursday",
"friday",
"saturday"
};

static const char* const DAYS[] = {
"",
"the first",
"the second",
"the third",
"the fourth",
"the fifth",
"the sixth",
"the seventh",
"the eighth",
"the ninth",
"the tenth",
"the eleventh",
"the twelfth",
"the thirteenth",
"the fourteenth",
"the fifteenth",
"the sixteenth",
"the seventeenth",
"the eighteenth",
"the nineteenth",
"the twentieth",
"the twenty-first",
"the twenty-second",
"the twenty-third",
"the twenty-fourth",
"the twenty-fifth",
"the twenty-sixth",
"the twenty-seventh",
"the twenty-eighth",
"the twenty-ninth",
"the thirtieth",
"the thirty-first"
};

static bool textInitialized = false;

// Animation handler
void animationStoppedHandler(struct Animation *animation, bool finished, void *context)
{
	TextLayer *current = (TextLayer *)context;
	GRect rect = layer_get_frame(&current->layer);
	rect.origin.x = 144;
	layer_set_frame(&current->layer, rect);
}

// Animate line
void makeAnimationsForLayers(Line *line, TextLayer *current, TextLayer *next)
{
	GRect rect = layer_get_frame(&next->layer);
	rect.origin.x -= 144;
	
	property_animation_init_layer_frame(&line->nextAnimation, &next->layer, NULL, &rect);
	animation_set_duration(&line->nextAnimation.animation, 400);
	animation_set_curve(&line->nextAnimation.animation, AnimationCurveEaseOut);
	animation_schedule(&line->nextAnimation.animation);
	
	GRect rect2 = layer_get_frame(&current->layer);
	rect2.origin.x -= 144;
	
	property_animation_init_layer_frame(&line->currentAnimation, &current->layer, NULL, &rect2);
	animation_set_duration(&line->currentAnimation.animation, 400);
	animation_set_curve(&line->currentAnimation.animation, AnimationCurveEaseOut);
	
	animation_set_handlers(&line->currentAnimation.animation, (AnimationHandlers) {
		.stopped = (AnimationStoppedHandler)animationStoppedHandler
	}, current);
	
	animation_schedule(&line->currentAnimation.animation);
}

// Update line
void updateLineTo(Line *line, char lineStr[2][BUFFER_SIZE], char *value)
{
	TextLayer *next, *current;
	
	GRect rect = layer_get_frame(&line->currentLayer.layer);
	current = (rect.origin.x == 0) ? &line->currentLayer : &line->nextLayer;
	next = (current == &line->currentLayer) ? &line->nextLayer : &line->currentLayer;
	
	// Update correct text only
	if (current == &line->currentLayer) {
		memset(lineStr[1], 0, BUFFER_SIZE);
		memcpy(lineStr[1], value, strlen(value));
		text_layer_set_text(next, lineStr[1]);
	} else {
		memset(lineStr[0], 0, BUFFER_SIZE);
		memcpy(lineStr[0], value, strlen(value));
		text_layer_set_text(next, lineStr[0]);
	}
	
	makeAnimationsForLayers(line, current, next);
}

// Check to see if the current line needs to be updated
bool needToUpdateLine(Line *line, char lineStr[2][BUFFER_SIZE], char *nextValue)
{
	char *currentStr;
	GRect rect = layer_get_frame(&line->currentLayer.layer);
	currentStr = (rect.origin.x == 0) ? lineStr[0] : lineStr[1];

	if (memcmp(currentStr, nextValue, strlen(nextValue)) != 0 ||
		(strlen(nextValue) == 0 && strlen(currentStr) != 0)) {
		return true;
	}
	return false;
}

// Update screen based on new time
void display_time(PblTm *t)
{
	// The current time text will be stored in the following 3 strings
	char textLine1[BUFFER_SIZE];
	char textLine2[BUFFER_SIZE];
	char textLine3[BUFFER_SIZE];

	time_to_3words(t->tm_hour, t->tm_min, textLine1, textLine2, textLine3, BUFFER_SIZE);
	
	if (needToUpdateLine(&line1, line1Str, textLine1)) {
		updateLineTo(&line1, line1Str, textLine1);	
	}
	if (needToUpdateLine(&line2, line2Str, textLine2)) {
		updateLineTo(&line2, line2Str, textLine2);	
	}
	if (needToUpdateLine(&line3, line3Str, textLine3)) {
		updateLineTo(&line3, line3Str, textLine3);	
	}

	//check if need to update day or date
	if(strcmp(topDayLayer.text, WEEKDAYS[t->tm_wday]) != 0) {
    		text_layer_set_text(&topDayLayer, WEEKDAYS[t->tm_wday]);
    		text_layer_set_text(&bottomDayLayer, DAYS[t->tm_mday]);
	}
}

// Update screen without animation first time we start the watchface
void display_initial_time(PblTm *t)
{
	time_to_3words(t->tm_hour, t->tm_min, line1Str[0], line2Str[0], line3Str[0], BUFFER_SIZE);

    	text_layer_set_text(&topDayLayer, WEEKDAYS[t->tm_wday]);
    	text_layer_set_text(&bottomDayLayer, DAYS[t->tm_mday]);
	
	text_layer_set_text(&line1.currentLayer, line1Str[0]);
	text_layer_set_text(&line2.currentLayer, line2Str[0]);
	text_layer_set_text(&line3.currentLayer, line3Str[0]);
}

// Configure the first line of text
void configureBoldLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SANSATION_BOLD_42)));
	text_layer_set_text_color(textlayer, GColorWhite);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, GTextAlignmentLeft);
}

// Configure for the 2nd and 3rd lines
void configureLightLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SANSATION_LIGHT_42)));
	//text_layer_set_font(textlayer, fonts_get_system_font(RESOURCE_ID_FONT_SANSATION_LIGHT_42));
	text_layer_set_text_color(textlayer, GColorWhite);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, GTextAlignmentLeft);
}

// Configure for top day of week
void configureDayOfWeek(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	text_layer_set_text_color(textlayer, GColorWhite);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, GTextAlignmentRight);
}

// Configure for bottom day of month
void configureDayOfMonth(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_color(textlayer, GColorWhite);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, GTextAlignmentRight);
}

/** 
 * Debug methods. For quickly debugging enable debug macro on top to transform the watchface into
 * a standard app and you will be able to change the time with the up and down buttons
 */ 
#if DEBUG

void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	(void)recognizer;
	(void)window;
	
	t.tm_min += 1;
	if (t.tm_min >= 60) {
		t.tm_min = 0;
		t.tm_hour += 1;
		
		if (t.tm_hour >= 24) {
			t.tm_hour = 0;
		}
	}
	display_time(&t);
}

void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	(void)recognizer;
	(void)window;
	
	t.tm_min -= 1;
	if (t.tm_min < 0) {
		t.tm_min = 59;
		t.tm_hour -= 1;
	}
	display_time(&t);
}

void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}

#endif

void handle_init(AppContextRef ctx) {
  	(void)ctx;

	window_init(&window, "TextWatch");
	window_stack_push(&window, true);
	window_set_background_color(&window, GColorBlack);

	// Init resources
	resource_init_current_app(&APP_RESOURCES);
	
	// 1st line layers
	text_layer_init(&line1.currentLayer, GRect(0, 8, 144, 50));
	text_layer_init(&line1.nextLayer, GRect(144, 8, 144, 50));
	configureBoldLayer(&line1.currentLayer);
	configureBoldLayer(&line1.nextLayer);

	// 2nd layers
	text_layer_init(&line2.currentLayer, GRect(0, 45, 144, 50));
	text_layer_init(&line2.nextLayer, GRect(144, 45, 144, 50));
	configureLightLayer(&line2.currentLayer);
	configureLightLayer(&line2.nextLayer);

	// 3rd layers
	text_layer_init(&line3.currentLayer, GRect(0, 82, 144, 50));
	text_layer_init(&line3.nextLayer, GRect(144, 82, 144, 50));
	configureLightLayer(&line3.currentLayer);
	configureLightLayer(&line3.nextLayer);

	// top day layer
	text_layer_init(&topDayLayer, GRect(0, 134, 144, 20));
	configureDayOfWeek(&topDayLayer);

	// bottom day layer
	text_layer_init(&bottomDayLayer, GRect(0, 147, 144, 20));
	configureDayOfMonth(&bottomDayLayer);

	// Configure time on init
	get_time(&t);
	display_initial_time(&t);
	
	// Load layers
  	layer_add_child(&window.layer, &line1.currentLayer.layer);
	layer_add_child(&window.layer, &line1.nextLayer.layer);
	layer_add_child(&window.layer, &line2.currentLayer.layer);
	layer_add_child(&window.layer, &line2.nextLayer.layer);
	layer_add_child(&window.layer, &line3.currentLayer.layer);
	layer_add_child(&window.layer, &line3.nextLayer.layer);
	layer_add_child(&window.layer, &topDayLayer.layer);
	layer_add_child(&window.layer, &bottomDayLayer.layer);
	
#if DEBUG
	// Button functionality
	window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);
#endif
}

// Time handler called every minute by the system
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)ctx;

  display_time(t->tick_time);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
	.tick_info = {
		      .tick_handler = &handle_minute_tick,
		      .tick_units = MINUTE_UNIT
		    }
  };
  app_event_loop(params, &handlers);
}
