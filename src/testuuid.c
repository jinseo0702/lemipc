#include <stdio.h>
#include <stdlib.h> // rand(), srand()
#include <time.h>   // time()

#define COUNT 256   // 생성할 숫자의 개수

typedef struct s_pos {
    int x;
    int y;
} t_pos;

int main() {
    t_pos pos[COUNT];
    int i, rand_idx, j;
    t_pos temp_pos;

    // 1. 난수 생성 초기화 (현재 시간을 시드로 설정)
    srand((unsigned int)time(NULL));

    // 2. 배열에 1부터 16까지 순서대로 값 채우기
    // (만약 0부터 15를 원하면 numbers[i] = i; 로 변경)
    int k = 0;
    for (i = 0; i < 16; i++) {
      for (j = 0; j < 16; j++) {
        pos[k++] = (t_pos){i, j};
      }
    }

    // 3. 피셔-예이츠 셔플 (Fisher-Yates Shuffle) 알고리즘 적용
    // 배열을 뒤에서부터 순회하며 임의의 앞쪽 요소와 위치를 바꿈
    for (i = COUNT - 1; i > 0; i--) {
        rand_idx = rand() % (i + 1); // 0부터 i 사이의 랜덤 인덱스 생성

        // Swap (두 변수의 값을 교환)
        temp_pos = pos[i];
        pos[i] = pos[rand_idx];
        pos[rand_idx] = temp_pos;
    }

    // 4. 결과 출력
    printf("중복 없는 난수 %d개 생성 결과:\n", COUNT);
    for (i = 0; i < COUNT; i++) {
        printf("%2d %2d | ", pos[i].x, pos[i].y);
        if ((i + 1) % 16 == 0) {
          printf("\n");
        }
      }
      printf("\n");

    return 0;
}