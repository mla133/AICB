UPDATE recipe_inj_config SET inj_no=2 WHERE recipe_no=1 AND recipe_inj_config_id=2;
UPDATE recipe_inj_config SET inj_vol=5.0 WHERE recipe_no=1 AND recipe_inj_config_id=2;
UPDATE recipe_inj_config SET inj_rate=5.0 WHERE recipe_no=1 AND recipe_inj_config_id=2;
UPDATE recipe_inj_config SET inj_prods=1 WHERE recipe_no=1 AND recipe_inj_config_id=2;
SELECT * FROM recipe_inj_config WHERE recipe_inj_config_id=2;
UPDATE injector_config SET inj_tag="Titan #1" where injector_config_id=2;
UPDATE injector_config SET inj_type=3 where injector_config_id=2;
UPDATE injector_config SET inj_arm=0 where injector_config_id=2;
UPDATE injector_config SET inj_plumbing=1 where injector_config_id=2;
UPDATE injector_config SET inj_address=61 where injector_config_id=2;
SELECT * FROM injector_config WHERE injector_config_id=2;
UPDATE serial_port_config set comm_protocol=6 where serial_port_config_id=2;
UPDATE serial_port_config set baud_rate=1 where serial_port_config_id=2;
UPDATE serial_port_config set data_parity=3 where serial_port_config_id=2;
SELECT * FROM serial_port_config WHERE serial_port_config_id=2;
.quit
