.headers ON
.timeout 10000
update batch_setup set recipe_no=1, entered_preset=1000 where arm_no=1;
.quit
