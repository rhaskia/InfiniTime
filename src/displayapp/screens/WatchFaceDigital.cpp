#include "displayapp/screens/WatchFaceDigital.h"

#include <lvgl/lvgl.h>
#include <cstdio>

#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/WeatherSymbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/settings/Settings.h"
#include "components/ble/MusicService.h"

using namespace Pinetime::Applications::Screens;

static void event_handler(lv_obj_t* obj, lv_event_t event) {
  WatchFaceDigital* screen = static_cast<WatchFaceDigital*>(obj->user_data);
  screen->OnObjectEvent(obj, event);
}


WatchFaceDigital::WatchFaceDigital(Controllers::DateTime& dateTimeController,
                                   const Controllers::Battery& batteryController,
                                   const Controllers::Ble& bleController,
                                   Controllers::NotificationManager& notificationManager,
                                   Controllers::Settings& settingsController,
                                   Controllers::HeartRateController& heartRateController,
                                   Controllers::MotionController& motionController,
                                   Controllers::SimpleWeatherService& weatherService,
                                   Controllers::MusicService& musicService)
  : currentDateTime {},
    dateTimeController {dateTimeController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    weatherService {weatherService},
    statusIcons(batteryController, bleController),
    musicService(musicService)
{

  lv_obj_t* label;

  lv_style_init(&btn_style);
  lv_style_set_radius(&btn_style, LV_STATE_DEFAULT, 20);
  lv_style_set_bg_color(&btn_style, LV_STATE_DEFAULT, LV_COLOR_AQUA);
  lv_style_set_bg_opa(&btn_style, LV_STATE_DEFAULT, LV_OPA_50);

  statusIcons.Create();

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  weatherIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x999999));
  lv_obj_set_style_local_text_font(weatherIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &fontawesome_weathericons);
  lv_label_set_text(weatherIcon, "");
  lv_obj_align(weatherIcon, nullptr, LV_ALIGN_IN_TOP_MID, -20, 50);
  lv_obj_set_auto_realign(weatherIcon, true);

  temperature = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(temperature, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x999999));
  lv_label_set_text(temperature, "");
  lv_obj_align(temperature, nullptr, LV_ALIGN_IN_TOP_MID, 20, 50);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 0);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x999999));

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x999999));
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 0);

  label_time_ampm = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(label_time_ampm, "");
  lv_obj_align(label_time_ampm, lv_scr_act(), LV_ALIGN_IN_TOP_MID, -40, -55);

  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
  lv_label_set_text_static(heartbeatValue, "");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FFE7));
  lv_label_set_text_static(stepValue, "0");
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);

  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FFE7));
  lv_label_set_text_static(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, stepValue, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  
  btnPrev = lv_btn_create(lv_scr_act(), nullptr);
  btnPrev->user_data = this;
  lv_obj_set_event_cb(btnPrev, event_handler);
  lv_obj_set_size(btnPrev, 76, 76);
  lv_obj_align(btnPrev, nullptr, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
  lv_obj_add_style(btnPrev, LV_STATE_DEFAULT, &btn_style);
  label = lv_label_create(btnPrev, nullptr);
  lv_label_set_text_static(label, Symbols::stepBackward);

  btnNext = lv_btn_create(lv_scr_act(), nullptr);
  btnNext->user_data = this;
  lv_obj_set_event_cb(btnNext, event_handler);
  lv_obj_set_size(btnNext, 76, 76);
  lv_obj_align(btnNext, nullptr, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
  lv_obj_add_style(btnNext, LV_STATE_DEFAULT, &btn_style);
  label = lv_label_create(btnNext, nullptr);
  lv_label_set_text_static(label, Symbols::stepForward);

  btnPlayPause = lv_btn_create(lv_scr_act(), nullptr);
  btnPlayPause->user_data = this;
  lv_obj_set_event_cb(btnPlayPause, event_handler);
  lv_obj_set_size(btnPlayPause, 76, 76);
  lv_obj_align(btnPlayPause, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
  lv_obj_add_style(btnPlayPause, LV_STATE_DEFAULT, &btn_style);
  txtPlayPause = lv_label_create(btnPlayPause, nullptr);
  lv_label_set_text_static(txtPlayPause, Symbols::play);

  txtTrackDuration = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_long_mode(txtTrackDuration, LV_LABEL_LONG_SROLL);
  lv_obj_align(txtTrackDuration, nullptr, LV_ALIGN_IN_TOP_LEFT, 12, 20);
  lv_label_set_text_static(txtTrackDuration, "--:--/--:--");
  lv_label_set_align(txtTrackDuration, LV_ALIGN_IN_LEFT_MID);
  lv_obj_set_width(txtTrackDuration, LV_HOR_RES);

  constexpr uint8_t FONT_HEIGHT = 12;
  constexpr uint8_t LINE_PAD = 15;
  constexpr int8_t MIDDLE_OFFSET = -25;
  txtArtist = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_long_mode(txtArtist, LV_LABEL_LONG_SROLL_CIRC);
  lv_obj_align(txtArtist, nullptr, LV_ALIGN_IN_LEFT_MID, 12, MIDDLE_OFFSET + 1 * FONT_HEIGHT);
  lv_label_set_align(txtArtist, LV_ALIGN_IN_LEFT_MID);
  lv_obj_set_width(txtArtist, LV_HOR_RES - 12);
  lv_label_set_text_static(txtArtist, "Artist Name");

  txtTrack = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_long_mode(txtTrack, LV_LABEL_LONG_SROLL_CIRC);
  lv_obj_align(txtTrack, nullptr, LV_ALIGN_IN_LEFT_MID, 12, MIDDLE_OFFSET + 2 * FONT_HEIGHT + LINE_PAD);

  lv_label_set_align(txtTrack, LV_ALIGN_IN_LEFT_MID);
  lv_obj_set_width(txtTrack, LV_HOR_RES - 12);
  lv_label_set_text_static(txtTrack, "This is a very long getTrack name");  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);

  musicService.event(Controllers::MusicService::EVENT_MUSIC_OPEN);

  Refresh();
}

WatchFaceDigital::~WatchFaceDigital() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
  lv_style_reset(&btn_style);
}

void WatchFaceDigital::UpdateLength() {
  if (totalLength > (99 * 60 * 60)) {
    lv_label_set_text_static(txtTrackDuration, "Inf/Inf");
  } else if (totalLength > (99 * 60)) {
    lv_label_set_text_fmt(txtTrackDuration,
                          "%02d:%02d/%02d:%02d",
                          (currentPosition / (60 * 60)) % 100,
                          ((currentPosition % (60 * 60)) / 60) % 100,
                          (totalLength / (60 * 60)) % 100,
                          ((totalLength % (60 * 60)) / 60) % 100);
  } else {
    lv_label_set_text_fmt(txtTrackDuration,
                          "%02d:%02d/%02d:%02d",
                          (currentPosition / 60) % 100,
                          (currentPosition % 60) % 100,
                          (totalLength / 60) % 100,
                          (totalLength % 60) % 100);
  }
}

void WatchFaceDigital::Refresh() {
  statusIcons.Update();

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());

  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();

    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
      char ampmChar[3] = "AM";
      if (hour == 0) {
        hour = 12;
      } else if (hour == 12) {
        ampmChar[0] = 'P';
      } else if (hour > 12) {
        hour = hour - 12;
        ampmChar[0] = 'P';
      }
      lv_label_set_text(label_time_ampm, ampmChar);
      lv_label_set_text_fmt(label_time, "%2d:%02d", hour, minute);
      lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 0);
    } else {
      // lv_label_set_text_fmt(label_time, "%02d:%02d", hour, minute);
      track = musicService.getTrack();
      lv_label_set_text(label_time, track.data());
      lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 0);
    }

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint16_t year = dateTimeController.Year();
      uint8_t day = dateTimeController.Day();
      if (settingsController.GetClockType() == Controllers::Settings::ClockType::H24) {
      track = musicService.getTrack();
      lv_label_set_text(label_time, track.data());
      lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 0);
        // lv_label_set_text_fmt(label_date,
        //                       "%s %d %s %d",
        //                       dateTimeController.DayOfWeekShortToString(),
        //                       day,
        //                       dateTimeController.MonthShortToString(),
        //                       year);
      } else {
        lv_label_set_text_fmt(label_date,
                              "%s %s %d %d",
                              dateTimeController.DayOfWeekShortToString(),
                              dateTimeController.MonthShortToString(),
                              day,
                              year);
      }
      lv_obj_realign(label_date);
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    } else {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x1B1B1B));
      lv_label_set_text_static(heartbeatValue, "");
    }

    lv_obj_realign(heartbeatIcon);
    lv_obj_realign(heartbeatValue);
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
    lv_obj_realign(stepValue);
    lv_obj_realign(stepIcon);
  }

  currentWeather = weatherService.Current();
  if (currentWeather.IsUpdated()) {
    auto optCurrentWeather = currentWeather.Get();
    if (optCurrentWeather) {
      int16_t temp = optCurrentWeather->temperature.Celsius();
      char tempUnit = 'C';
      if (settingsController.GetWeatherFormat() == Controllers::Settings::WeatherFormat::Imperial) {
        temp = optCurrentWeather->temperature.Fahrenheit();
        tempUnit = 'F';
      }
      lv_label_set_text_fmt(temperature, "%d°%c", temp, tempUnit);
      lv_label_set_text(weatherIcon, Symbols::GetSymbol(optCurrentWeather->iconId));
    } else {
      lv_label_set_text_static(temperature, "");
      lv_label_set_text(weatherIcon, "");
    }
    lv_obj_realign(temperature);
    lv_obj_realign(weatherIcon);
  }

  if (artist != musicService.getArtist()) {
    artist = musicService.getArtist();
    lv_label_set_text(txtArtist, artist.data());
  }

  if (track != musicService.getTrack()) {
    track = musicService.getTrack();
    lv_label_set_text(txtTrack, track.data());
  }

  if (album != musicService.getAlbum()) {
    album = musicService.getAlbum();
  }

  if (playing != musicService.isPlaying()) {
    playing = musicService.isPlaying();
  }

  if (currentPosition != musicService.getProgress()) {
    currentPosition = musicService.getProgress();
    UpdateLength();
  }

  if (totalLength != musicService.getTrackLength()) {
    totalLength = musicService.getTrackLength();
    UpdateLength();
  }

  if (playing) {
    lv_label_set_text_static(txtPlayPause, Symbols::pause);
    if (xTaskGetTickCount() - 1024 >= lastIncrement) {
      if (currentPosition >= totalLength) {
        // Let's assume the getTrack finished, paused when the timer ends
        //  and there's no new getTrack being sent to us
        playing = false;
      }
      lastIncrement = xTaskGetTickCount();
    }
  } else {
    lv_label_set_text_static(txtPlayPause, Symbols::play);
  }
}

void WatchFaceDigital::OnObjectEvent(lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_CLICKED) {
    if (obj == btnPrev) {
      musicService.event(Controllers::MusicService::EVENT_MUSIC_PREV);
    } else if (obj == btnPlayPause) {
      if (playing == Pinetime::Controllers::MusicService::MusicStatus::Playing) {
        musicService.event(Controllers::MusicService::EVENT_MUSIC_PAUSE);

        // Let's assume it stops playing instantly
        playing = Controllers::MusicService::NotPlaying;
      } else {
        musicService.event(Controllers::MusicService::EVENT_MUSIC_PLAY);

        // Let's assume it starts playing instantly
        // TODO: In the future should check for BT connection for better UX
        playing = Controllers::MusicService::Playing;
      }
    } else if (obj == btnNext) {
      musicService.event(Controllers::MusicService::EVENT_MUSIC_NEXT);
    }
  }
}

