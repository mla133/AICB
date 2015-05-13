.headers ON
.mode csv
.output DB_out.csv
select * from system_config;
select * from meter_config where meter_config_id=1;
select * from product_config;
select * from ticket_config;
select * from tax_category;
select * from tax_category_taxes;
select * from vol_discount_category;
select * from misc_charge;
select * from vol_discount;
select * from customer_category;
select * from serial_port_config;
select * from dig_in_config where meter_id=1;
select * from dig_out_config where meter_id=1;
select * from user_account;
