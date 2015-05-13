.headers ON
-- Poll Batch Data
select arm_no from batch_data where batch_data_id=1;
select batch_no from batch_data where batch_data_id=1;
select recipe_no from batch_data where batch_data_id=1;
select preset_vol from batch_data where batch_data_id=1;
select remain_vol from batch_data where batch_data_id=1;
select batch_status from arm_live_data where arm_live_data_id=1;
select pump_status from arm_live_data where arm_live_data_id=1;
select batch_iv from batch_data where batch_data_id=1;
select batch_add1 from batch_data where batch_data_id=1;
select batch_add2 from batch_data where batch_data_id=1;
select batch_add3 from batch_data where batch_data_id=1;
select batch_add4 from batch_data where batch_data_id=1;
select batch_add5 from batch_data where batch_data_id=1;
select batch_add6 from batch_data where batch_data_id=1;
.quit
