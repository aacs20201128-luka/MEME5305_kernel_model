#ifndef MARIADB_H   //→ 如果還沒定義
#define MARIADB_H   //→ 就先定義

int connection_mariadb();

int disconnection_mariadb();

int insert_dht11_mariadb(unsigned char temperature[], unsigned char humidity[]);

int select_latest_dht11_data();

int insert_bh1750_mariadb(unsigned char lux[]);

int select_latest_bh1750_data();

#endif  //→ 結束判斷