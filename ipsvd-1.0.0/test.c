#include "dns.h"

int main () {
        stralloc out ={0};
        stralloc sa ={0};
        char ip[4];
        char *dn =0;
        
        stralloc_copys(&sa, "abcdefg");

        dns_ip4(&out, &sa);
        dns_ip4_qualify(&out, &sa, &sa);
        dns_name4(&out, ip);
        dns_mx(&out, &sa);
        dns_txt(&out, &sa);

        dns_domain_length(sa.s);
        dns_domain_equal(sa.s, sa.s);
        dns_domain_copy(&dn, sa.s);
        dns_domain_fromdot(&dn, sa.s, sa.len);

        return 0;
}
