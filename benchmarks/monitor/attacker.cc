// void attack()
// {
//     my_unlink("/tmp");
//     my_symlink("/", "/tmp");
//     my_unlink("/tmp");
//     my_symlink("/tmp2", "/tmp");
//     my_unlink("/tmp2");
//     my_symlink("/etc", "/tmp2");

//     my_unlink("/tmp");
//     my_symlink("/", "/tmp");
//     my_unlink("/tmp");
//     my_symlink("/tmp2", "/tmp");
//     my_unlink("/tmp2");
//     my_symlink("/etc", "/tmp2");
//     my_unlink("/tmp2");
//     my_symlink("/etc", "/tmp2");
// }

#include "mockfs.h"
#include "attacker.h"

void init_attack()
{
    input[0] = "2/3/6";
    attack[0] = []()
    {
        unlink_and_symlink("0", "0");
        unlink_and_symlink("0/-1/-1", "");
        unlink_and_symlink("-1", "-1");
        unlink_and_symlink("-1/-1", "-1/-1");
    };
}
