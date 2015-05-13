.headers ON
update batch_setup set recipe_no=1 where arm_no=1;
update batch_setup set entered_preset=100 where arm_no=1;
.quit
