#ifndef MARIADB_H   //→ 如果還沒定義
#define MARIADB_H   //→ 就先定義

int connection_mariadb();

int disconnection_mariadb();

int insert_mariadb(unsigned char temperature[], unsigned char humidity[]);

int selectAll_dht11_data();

#endif  //→ 結束判斷