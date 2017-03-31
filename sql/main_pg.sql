
--DROP SCHEMA if exists regsmp CASCADE;

CREATE SCHEMA regsmp;

CREATE TABLE regsmp.config
(
  app_class character varying(32) NOT NULL,
  db_public character varying(256) NOT NULL,
  udp_port character varying(32) NOT NULL,
  pid_path character varying(32) NOT NULL,
  udp_buf_size character varying(32) NOT NULL,--size of buffer used in sendto() and recvfrom() functions (508 is safe, if more then packet may be lost while transferring over network). Enlarge it if your rio module returns SRV_BUF_OVERFLOW
  db_data character varying(32) NOT NULL,
  cycle_duration_us character varying(32) NOT NULL,--the more cycle duration is, the less your hardware is loaded
  peer_lock character varying(32) NOT NULL,
  CONSTRAINT config_pkey PRIMARY KEY (app_class)
)
WITH (
  OIDS=FALSE
);

CREATE TABLE regsmp.prog
(
  id integer NOT NULL,
  description character varying(32) NOT NULL DEFAULT 'prog',
  sensor_id integer NOT NULL DEFAULT -1,
  em_heater_id integer NOT NULL DEFAULT -1,
  em_cooler_id integer NOT NULL DEFAULT -1,
  goal real NOT NULL DEFAULT 20.0,
  mode character(3) DEFAULT 'pid', --pid or onf mode
  heater_pid_id integer NOT NULL DEFAULT -1,
  cooler_pid_id integer NOT NULL DEFAULT -1,
  onf_id integer NOT NULL DEFAULT -1,
  change_gap_id integer NOT NULL DEFAULT -1,--time from regsmp.gap for switching EM (heater or cooler)
  active integer NOT NULL DEFAULT 0,
  CONSTRAINT prog_pkey PRIMARY KEY (id),
  CONSTRAINT prog_mode_check CHECK (mode = 'pid'::bpchar OR mode = 'onf'::bpchar)
)
WITH (
  OIDS=FALSE
);

CREATE TABLE regsmp.pid
(
  id integer NOT NULL,
  kp real DEFAULT 100,
  ki real DEFAULT 100,
  kd real DEFAULT 100,
  CONSTRAINT pid_pkey PRIMARY KEY (id)
)
WITH (
  OIDS=FALSE
);

CREATE TABLE regsmp.onf
(
  id integer NOT NULL,
  delta real DEFAULT 0.5,
  CONSTRAINT onf_pkey PRIMARY KEY (id)
)
WITH (
  OIDS=FALSE
);

CREATE TABLE regsmp.gap
(
  id integer NOT NULL,
  amount interval NOT NULL DEFAULT '1800',
  CONSTRAINT gap_pkey PRIMARY KEY (id)
)
WITH (
  OIDS=FALSE
);

CREATE TABLE regsmp.sensor_mapping
(
  app_class character varying(32) NOT NULL,
  sensor_id integer NOT NULL,
  peer_id character varying(32) NOT NULL,
  remote_id integer NOT NULL,
  CONSTRAINT sensor_mapping_pkey PRIMARY KEY (app_class, sensor_id, peer_id, remote_id)
)
WITH (
  OIDS=FALSE
);

CREATE TABLE regsmp.em_mapping
(
  app_class character varying(32) NOT NULL,
  em_id integer NOT NULL,
  peer_id character varying(32) NOT NULL,
  remote_id integer NOT NULL,
  pwm_rsl real NOT NULL DEFAULT 100.0, --max duty cycle value (see lib/pid.h PWM_RSL)
  CONSTRAINT em_mapping_pkey PRIMARY KEY (app_class, em_id, peer_id, remote_id)
)
WITH (
  OIDS=FALSE
);

