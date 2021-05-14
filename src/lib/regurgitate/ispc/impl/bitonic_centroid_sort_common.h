#ifndef _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_COMMON_H_
#define _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_COMMON_H_

struct centroid
{
    key_type keys;
    int_val_type vals;
};

static inline unmasked void do_minmax(struct centroid& a, struct centroid& b)
{
    key_type tmp = a.keys;
    a.keys = min(a.keys, b.keys);
    b.keys = max(b.keys, tmp);

    int_val_type mask = select(a.keys != tmp, sign_extend(true), 0);
    int_val_type q = (a.vals ^ b.vals) & mask;
    a.vals ^= q;
    b.vals ^= q;
}

static inline unmasked struct centroid
shuffle(const struct centroid& a, const struct centroid& b, int mask)
{
    struct centroid c = {shuffle(a.keys, b.keys, mask),
                         shuffle(a.vals, b.vals, mask)};
    return (c);
}

static inline unmasked struct centroid shuffle(const struct centroid& a,
                                               int mask)
{
    struct centroid c = {shuffle(a.keys, mask), shuffle(a.vals, mask)};
    return (c);
}

#endif /* _REGURGITATE_IMPL_BITONIC_CENTROID_SORT_COMMON_H_ */
