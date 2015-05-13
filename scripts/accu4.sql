-- pDB_edit_buf commands
update inj_sys_config set inj_units_desc='cc', inj_totals_desc='gal', inj_conversion_factor=3785.412;
update injector_config set inj_tag='Injector 1', inj_type=1, inj_arm=1, inj_plumbing=1 where injector_config_id=1;
update recipe_inj_config set inj_no=1, inj_vol=10.0, inj_rate=25.0, inj_prods=1 where recipe_no=1 and recipe_inj_config_id=1;
update dig_out_config set dig_out_tag='Injector #1', dig_out_func=33, dig_out_arm=1, dig_out_mtr=1, dig_out_prd=1 where dig_out_config_id=1;

-- Check injector configs (pDB_edit_buf)
select * from injector_config where injector_config_id<5;
select inj_units_desc, inj_totals_desc, inj_conversion_factor from inj_sys_config;
select * from recipe_inj_config where inj_no=1;

-- accu4_db_ram commands
insert into delivery_queue (delivery_cmd, arm_no) values ('Prg Mode Login',0); -- Login
insert into delivery_queue (delivery_cmd, arm_no) values ('Prg Mode Abort',0); -- Abort
insert into delivery_queue (delivery_cmd, arm_no) values ('Prg Mode Logout',0); -- Logout

update batch_setup set recipe_no=1, entered_preset=200 where arm_no=1;

insert into delivery_queue (delivery_cmd, arm_no) values ('Start',1); -- start
insert into delivery_queue (delivery_cmd, arm_no) values ('Stop Arm',1); -- stop Arm1
insert into delivery_queue (delivery_cmd, arm_no) values ('End Trans',1); -- End Transaction

select * from bool_arm_status where Bool_arm_status_id=1; --Poll flags for transaction/batch status

select * from batch_data where batch_data_id=1; -- Poll Batch Data
select * from trans_arm_data where trans_arm_data_id=1; --Poll Transaction Data

select parameter_db_busy from system;