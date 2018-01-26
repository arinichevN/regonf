delete from em_mapping;
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (1,'obj_1',1,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (2,'obj_1',2,1000);

INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (3,'obj_1',3,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (4,'obj_1',4,1000);

INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (5,'obj_1',5,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (6,'obj_1',6,1000);

INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (7,'obj_1',7,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (8,'obj_1',8,1000);

delete from sensor_mapping;
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (1,'obj_1',1);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (2,'obj_1',2);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (3,'obj_1',3);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (4,'obj_1',4);

delete from secure;
INSERT INTO secure (id,timeout_sec,heater_duty_cycle,cooler_duty_cycle) VALUES(2,30,0,0);

delete from prog;
INSERT INTO prog(id,description,em_mode,sensor_id,heater_em_id,cooler_em_id,goal,heater_delta,cooler_delta,change_gap,secure_id,save,enable,load) VALUES 
(1,'регулятор1_канал1','both',1,1,2,25,0.5,0.5,1800,1,1,1,1);
INSERT INTO prog(id,description,em_mode,sensor_id,heater_em_id,cooler_em_id,goal,heater_delta,cooler_delta,change_gap,secure_id,save,enable,load) VALUES
(2,'регулятор1_канал2','both',2,3,4,25,0.5,0.5,1800,1,1,1,1);
INSERT INTO prog(id,description,em_mode,sensor_id,heater_em_id,cooler_em_id,goal,heater_delta,cooler_delta,change_gap,secure_id,save,enable,load) VALUES
(3,'регулятор1_канал3','both',3,5,6,25,0.5,0.5,1800,1,1,1,1);
INSERT INTO prog(id,description,em_mode,sensor_id,heater_em_id,cooler_em_id,goal,heater_delta,cooler_delta,change_gap,secure_id,save,enable,load) VALUES
(4,'регулятор1_канал4','both',4,7,8,25,0.5,0.5,1800,1,1,1,1);





