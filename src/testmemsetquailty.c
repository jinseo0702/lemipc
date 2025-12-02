#include "../libft/libft.h"
#include "../printf/libftprintf.h"

int main() {
    char str[16][16];
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) {
        ft_fprintf(1, "%d ", str[i][j]);
      }
      ft_fprintf(1, "\n");
    }
    ft_memset(str, 0, sizeof(str));
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) {
        ft_fprintf(1, "%d ", str[i][j]);
      }
      ft_fprintf(1, "\n");
    }
    return 0;
}