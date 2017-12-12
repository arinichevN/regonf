delete from em_mapping;
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (1,'gwu74_1',8,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (2,'gwu74_1',7,1000);

INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (3,'gwu74_1',6,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (4,'gwu74_1',5,1000);

INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (5,'gwu74_1',4,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (6,'gwu74_1',3,1000);

INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (7,'gwu74_1',2,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (8,'gwu74_1',1,1000);

delete from sensor_mapping;
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (1,'gwu18_1',1);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (2,'gwu18_1',2);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (3,'gwu18_1',3);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (4,'gwu18_1',4);


delete from prog;
INSERT INTO prog(id,description,em_mode,sensor_id,heater_em_id,cooler_em_id,goal,heater_delta,cooler_delta,change_gap,enable,load) VALUES 
(1,'регулятор1_канал1','both',1,1,5,25,0.5,0.5,1800,1,1);
INSERT INTO prog(id,description,em_mode,sensor_id,heater_em_id,cooler_em_id,goal,heater_delta,cooler_delta,change_gap,enable,load) VALUES
(2,'регулятор1_канал2','both',2,2,6,25,0.5,0.5,1800,1,1);
INSERT INTO prog(id,description,em_mode,sensor_id,heater_em_id,cooler_em_id,goal,heater_delta,cooler_delta,change_gap,enable,load) VALUES
(3,'регулятор1_канал3','both',3,3,7,25,0.5,0.5,1800,1,1);
INSERT INTO prog(id,description,em_mode,sensor_id,heater_em_id,cooler_em_id,goal,heater_delta,cooler_delta,change_gap,enable,load) VALUES
(4,'регулятор1_канал4','both',4,4,8,25,0.5,0.5,1800,1,1);




