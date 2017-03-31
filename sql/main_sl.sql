
CREATE TABLE "prog"
(
  "id" INTEGER PRIMARY KEY,
  "description" TEXT NOT NULL,
  "sensor_id" INTEGER NOT NULL,
  "em_heater_id" INTEGER NOT NULL,
  "em_cooler_id" INTEGER NOT NULL,
  "goal" REAL NOT NULL,
  "delta" REAL NOT NULL,
  "change_gap" INTEGER NOT NULL,--time from regsmp.gap for switching EM (heater or cooler)
  "enable" INTEGER NOT NULL,
  "load" INTEGER NOT NULL
);

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

