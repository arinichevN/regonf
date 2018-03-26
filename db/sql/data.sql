CREATE TABLE "sensor_mapping" (
    "sensor_id" INTEGER PRIMARY KEY NOT NULL,
    "peer_id" TEXT NOT NULL,
    "remote_id" INTEGER NOT NULL
);
CREATE TABLE "em_mapping" (
    "em_id" INTEGER PRIMARY KEY NOT NULL,
    "peer_id" TEXT NOT NULL,
    "remote_id" INTEGER NOT NULL,
    "pwm_rsl" REAL NOT NULL
);
CREATE TABLE "prog"
(
  "id" INTEGER PRIMARY KEY,
  "description" TEXT NOT NULL,
  "sensor_id" INTEGER NOT NULL,
  "goal" REAL NOT NULL,
  "change_gap" INTEGER NOT NULL,--time from regsmp.gap for switching EM (heater or cooler), sec
  "em_mode" TEXT NOT NULL,--cooler or heater or both
  "heater_em_id" INTEGER NOT NULL,
  "heater_delta" REAL NOT NULL,
  "cooler_em_id" INTEGER NOT NULL,
  "cooler_delta" REAL NOT NULL,
  "secure_id" INTEGER NOT NULL,
  "green_light_sensor_id" INTEGER NOT NULL,--regulator initialization will be completed only when this sensor value is equal to green_value (use it when you want your regulator to do work when your hard and software are ready)
  "green_value" REAL NOT NULL,
  "save" INTEGER NOT NULL,--write data to persistent storage or not
  "enable" INTEGER NOT NULL,
  "load" INTEGER NOT NULL
);

CREATE TABLE "secure"
(
  "id" INTEGER PRIMARY KEY,
  "timeout_sec" INTEGER NOT NULL, -- if secure_enable, we will set this pin to PWM mode with secure_duty_cycle after we have no requests to this pin while secure_timeout_sec is running
  "heater_duty_cycle" INTEGER NOT NULL, -- 0...rsl
  "cooler_duty_cycle" INTEGER NOT NULL -- 0...rsl
);