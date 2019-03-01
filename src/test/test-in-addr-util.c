/* SPDX-License-Identifier: LGPL-2.1+ */

#include <fnmatch.h>
#include <netinet/in.h>

#include "log.h"
#include "strv.h"
#include "in-addr-util.h"

static void test_in_addr_prefix_from_string(
                const char *p,
                int family,
                int ret,
                const union in_addr_union *u,
                unsigned char prefixlen,
                int ret_refuse,
                unsigned char prefixlen_refuse,
                int ret_legacy,
                unsigned char prefixlen_legacy) {

        union in_addr_union q;
        unsigned char l;
        int f, r;

        r = in_addr_prefix_from_string(p, family, &q, &l);
        assert_se(r == ret);

        if (r < 0)
                return;

        assert_se(in_addr_equal(family, &q, u));
        assert_se(l == prefixlen);

        r = in_addr_prefix_from_string_auto(p, &f, &q, &l);
        assert_se(r >= 0);

        assert_se(f == family);
        assert_se(in_addr_equal(family, &q, u));
        assert_se(l == prefixlen);

        r = in_addr_prefix_from_string_auto_internal(p, PREFIXLEN_REFUSE, &f, &q, &l);
        assert_se(r == ret_refuse);

        if (r >= 0) {
                assert_se(f == family);
                assert_se(in_addr_equal(family, &q, u));
                assert_se(l == prefixlen_refuse);
        }

        r = in_addr_prefix_from_string_auto_internal(p, PREFIXLEN_LEGACY, &f, &q, &l);
        assert_se(r == ret_legacy);

        if (r >= 0) {
                assert_se(f == family);
                assert_se(in_addr_equal(family, &q, u));
                assert_se(l == prefixlen_legacy);
        }
}

static void test_in_addr_random_prefix(void) {
        _cleanup_free_ char *str = NULL;
        union in_addr_union a;

        assert_se(in_addr_from_string(AF_INET, "192.168.10.1", &a) >= 0);

        assert_se(in_addr_random_prefix(AF_INET, &a, 31, 32) >= 0);
        assert_se(in_addr_to_string(AF_INET, &a, &str) >= 0);
        assert_se(STR_IN_SET(str, "192.168.10.0", "192.168.10.1"));
        str = mfree(str);

        assert_se(in_addr_random_prefix(AF_INET, &a, 24, 26) >= 0);
        assert_se(in_addr_to_string(AF_INET, &a, &str) >= 0);
        assert_se(startswith(str, "192.168.10."));
        str = mfree(str);

        assert_se(in_addr_random_prefix(AF_INET, &a, 16, 24) >= 0);
        assert_se(in_addr_to_string(AF_INET, &a, &str) >= 0);
        assert_se(fnmatch("192.168.[0-9]*.0", str, 0) == 0);
        str = mfree(str);

        assert_se(in_addr_random_prefix(AF_INET, &a, 8, 24) >= 0);
        assert_se(in_addr_to_string(AF_INET, &a, &str) >= 0);
        assert_se(fnmatch("192.[0-9]*.[0-9]*.0", str, 0) == 0);
        str = mfree(str);

        assert_se(in_addr_random_prefix(AF_INET, &a, 8, 16) >= 0);
        assert_se(in_addr_to_string(AF_INET, &a, &str) >= 0);
        assert_se(fnmatch("192.[0-9]*.0.0", str, 0) == 0);
        str = mfree(str);

        assert_se(in_addr_from_string(AF_INET6, "fd00::1", &a) >= 0);

        assert_se(in_addr_random_prefix(AF_INET6, &a, 16, 64) >= 0);
        assert_se(in_addr_to_string(AF_INET6, &a, &str) >= 0);
        assert_se(startswith(str, "fd00:"));
        str = mfree(str);

        assert_se(in_addr_random_prefix(AF_INET6, &a, 8, 16) >= 0);
        assert_se(in_addr_to_string(AF_INET6, &a, &str) >= 0);
        assert_se(fnmatch("fd??::", str, 0) == 0);
        str = mfree(str);
}

int main(int argc, char *argv[]) {
        test_in_addr_prefix_from_string("", AF_INET, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string("/", AF_INET, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string("/8", AF_INET, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string("1.2.3.4", AF_INET, 0, &(union in_addr_union) { .in = (struct in_addr) { .s_addr = htobe32(0x01020304) } }, 32, -ENOANO, 0, 0, 8);
        test_in_addr_prefix_from_string("1.2.3.4/0", AF_INET, 0, &(union in_addr_union) { .in = (struct in_addr) { .s_addr = htobe32(0x01020304) } }, 0, 0, 0, 0, 0);
        test_in_addr_prefix_from_string("1.2.3.4/1", AF_INET, 0, &(union in_addr_union) { .in = (struct in_addr) { .s_addr = htobe32(0x01020304) } }, 1, 0, 1, 0, 1);
        test_in_addr_prefix_from_string("1.2.3.4/2", AF_INET, 0, &(union in_addr_union) { .in = (struct in_addr) { .s_addr = htobe32(0x01020304) } }, 2, 0, 2, 0, 2);
        test_in_addr_prefix_from_string("1.2.3.4/32", AF_INET, 0, &(union in_addr_union) { .in = (struct in_addr) { .s_addr = htobe32(0x01020304) } }, 32, 0, 32, 0, 32);
        test_in_addr_prefix_from_string("1.2.3.4/33", AF_INET, -ERANGE, NULL, 0, -ERANGE, 0, -ERANGE, 0);
        test_in_addr_prefix_from_string("1.2.3.4/-1", AF_INET, -ERANGE, NULL, 0, -ERANGE, 0, -ERANGE, 0);
        test_in_addr_prefix_from_string("::1", AF_INET, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);

        test_in_addr_prefix_from_string("", AF_INET6, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string("/", AF_INET6, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string("/8", AF_INET6, -EINVAL, NULL, 0, -EINVAL, 0, -EINVAL, 0);
        test_in_addr_prefix_from_string("::1", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 128, -ENOANO, 0, 0, 0);
        test_in_addr_prefix_from_string("::1/0", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 0, 0, 0, 0, 0);
        test_in_addr_prefix_from_string("::1/1", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 1, 0, 1, 0, 1);
        test_in_addr_prefix_from_string("::1/2", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 2, 0, 2, 0, 2);
        test_in_addr_prefix_from_string("::1/32", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 32, 0, 32, 0, 32);
        test_in_addr_prefix_from_string("::1/33", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 33, 0, 33, 0, 33);
        test_in_addr_prefix_from_string("::1/64", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 64, 0, 64, 0, 64);
        test_in_addr_prefix_from_string("::1/128", AF_INET6, 0, &(union in_addr_union) { .in6 = IN6ADDR_LOOPBACK_INIT }, 128, 0, 128, 0, 128);
        test_in_addr_prefix_from_string("::1/129", AF_INET6, -ERANGE, NULL, 0, -ERANGE, 0, -ERANGE, 0);
        test_in_addr_prefix_from_string("::1/-1", AF_INET6, -ERANGE, NULL, 0, -ERANGE, 0, -ERANGE, 0);

        test_in_addr_random_prefix();

        return 0;
}
