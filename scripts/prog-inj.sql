UPDATE inj_sys_config SET inj_units_desc='cc';
UPDATE inj_sys_config SET inj_totals_desc='gal';
UPDATE inj_sys_config SET inj_conversion_factor=3785.412;

UPDATE recipe_inj_config SET inj_no=1;
UPDATE recipe_inj_config SET inj_vol=10.0;
UPDATE recipe_inj_config SET inj_rate=25.0;

UPDATE dig_out_config SET dig_out_tag='INJ1-ARM1';
UPDATE dig_out_config SET dig_out_arm=1;
UPDATE dig_out_config SET dig_out_mtr=1;
UPDATE dig_out_config SET dig_out_prd=1;

.quit
