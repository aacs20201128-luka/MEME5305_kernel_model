//一般 user-space C 裡，跨 .c 檔共享函式或變數，通常靠 .h 裡的宣告，變數才特別常看到 extern。

#ifndef SLAVE_H
#define SLAVE_H
void slave_test(void);//函式其實宣告本身就帶有外部連結的意思
#endif