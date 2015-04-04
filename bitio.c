#include "bitio.h"
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#define BITIO_BUF_WORDS 512

struct bitio {
    int bitio_fd;
    int bitio_mode;
    unsigned bitio_rp;
    unsigned bitio_wp;
    uint64_t buf[BITIO_BUF_WORDS];
};

struct bitio*
bitio_open(const char *fn, char mode)
{
    struct bitio *ret = NULL;

    if (fn == NULL || (mode != 'r' && mode != 'w')) {
        errno = EINVAL;
        goto fail;
    }

    ret = calloc(1, sizeof(struct bitio));
    if (ret == NULL) {
        goto fail;
    }

    ret->bitio_fd = open(fn, (mode == 'r' ? O_RDONLY : (O_WRONLY | O_CREAT)), 0666);
    if (ret->bitio_fd < 0) {
        goto fail;
    }

    ret->bitio_mode = mode;
    ret->bitio_rp = ret->bitio_wp = 0;

    return ret;

fail:
    if (ret != NULL) {
        ret->bitio_fd = 0;
        free(ret);
        ret = NULL;
    }

    return NULL;
}

int
bitio_flush(struct bitio *b)
{
    int len_bytes, i;
    char *start, *dst;
    int left;
    if (b == NULL || b->bitio_mode != 'w' || b->bitio_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    if (b->bitio_wp == 0) {
        return 0;
    }

    len_bytes = b->bitio_wp / 8;
    if (len_bytes == 0) {
        return 0;
    }

    start = (char *)(b->buf);
    left = len_bytes;
    for(;;) {
        int x = write(b->bitio_fd, start, left);
        if (x < 0) {
            goto fail;
        }

        left -= x;
        start += x;
        if (left == 0) {
            break;
        }
    }

    b->bitio_wp = b->bitio_wp % 8;
    if (b->bitio_wp != 0) {
        char *dst = (char *)(b->buf);
        dst[0] = start[0];
    }

    return len_bytes * 8;

fail:
    dst = (char *)(b->buf);
    for (i = 0; i < len_bytes - left; i++) {
        dst[i] = start[i];
    }
    if (b->bitio_wp % 8 != 0) {
        dst[i] = start[i];
    }

    return -1;
}

int
bitio_fill_buffer(struct bitio *b)
{
    if (b->bitio_rp == b->bitio_wp) {
        b->bitio_rp = b->bitio_wp = 0;
        for (;;) {
            int x = read(b->bitio_fd, b->buf, sizeof(b->buf));
            if (x < 0) {
                return -1;
            }
            else if (x > 0) {
                b->bitio_wp = x * 8;
                return b->bitio_wp;
            }
            else {
                return 0;
            }
        }
    }
    return b->bitio_wp - b->bitio_rp;
}

int
bitio_read_64(struct bitio *b, uint64_t *buf, int len)
{
    int pos, ofs, t_len1 = 0, t_len2 = 0, bits_in_buf;
    uint64_t k, d = 0, tmp;
    // Load file in b->buf if it is empty
    bits_in_buf = bitio_fill_buffer(b);
    if (bits_in_buf <= 0) {
        return bits_in_buf;
    }

    pos = b->bitio_rp / (sizeof(b->buf[0]) * 8);
    ofs = b->bitio_rp % (sizeof(b->buf[0]) * 8);
    tmp = le64toh(b->buf[pos]);

    // Requested data exceeds b->buf's element dimension
    if (ofs + len > sizeof(b->buf[0]) * 8) {
        t_len1 = sizeof(b->buf[0]) * 8 - ofs;
        len -= t_len1;

        k = (((uint64_t)1 << t_len1) - 1) << ofs;
        d |= (tmp & k) >> ofs;

        b->bitio_rp += t_len1;
        bits_in_buf -= t_len1;
        ofs = 0;
        pos++;
        tmp = le64toh(b->buf[pos]);
    }
    // No enough data in b->buf
    if (len > bits_in_buf) {
        t_len2 = bits_in_buf;
        len -= t_len2;

        k = (((uint64_t)1 << t_len2) - 1) << ofs;
        tmp = (tmp & k) >> ofs;
        tmp <<= t_len1;
        d |= tmp;

        b->bitio_rp += t_len2;
        bits_in_buf = bitio_fill_buffer(b);
        if (bits_in_buf <= 0) {
            if (bits_in_buf < 0) {
                return bits_in_buf;
            }
            *buf = d;
            return t_len1 + t_len2;
        }
        pos = 0;
        tmp = le64toh(b->buf[pos]);
        if (len > bits_in_buf) {
            len = bits_in_buf;
        }
    }

    k = len == 64 ? ~(uint64_t)0 : (((uint64_t)1 << len) - 1) << ofs;
    tmp = (tmp & k) >> ofs;
    tmp <<= (t_len1 + t_len2);
    d |= tmp;

    b->bitio_rp += len;
    *buf = d;
    return len + t_len1 + t_len2;
}

int
bitio_read(struct bitio *b, uint64_t *buf, int len)
{
    int bits = sizeof(uint64_t) * 8;
    int i = len / bits;
    int r = len % bits;
    int read = 0, count = 0, ret;
    while (i > 0) {
        ret = bitio_read_64(b, &buf[count], bits);
        if (ret <= 0) {
            if (ret == 0) {
                return read;
            }
            return ret;
        }
        read += ret;
        i--;
        count++;
    }
    if (r != 0) {
        ret = bitio_read_64(b, &buf[count], r);
        if (ret <= 0) {
            if (ret == 0) {
                return read;
            }
            return ret;
        }
        read += ret;
    }
    return read;
}

int
bitio_write_64(struct bitio *b, uint64_t *buf, int len)
{
    int pos, ofs, t_len = 0;
    uint64_t d = 0, k, tmp;
    // Data to store exceeds b->buf dimension
    if (b->bitio_wp + len > sizeof(b->buf) * 8) {
        if (bitio_flush(b) < 0) {
            return -1;
        }
    }

    pos = b->bitio_wp / (sizeof(b->buf[0]) * 8);
    ofs = b->bitio_wp % (sizeof(b->buf[0]) * 8);

    d = le64toh(b->buf[pos]);
    tmp = *buf;

    // Data to store exceeds b->buf's element dimension
    if (ofs + len > sizeof(b->buf[0]) * 8) {
        t_len = sizeof(b->buf[0]) * 8 - ofs;
        len -= t_len;

        k = ((uint64_t)1 << t_len) - 1;
        tmp = (tmp & k) << ofs;
        d = (d & ~(k << ofs)) | tmp;

        b->buf[pos] = htole64(d);
        b->bitio_wp += t_len;
        pos++;
        ofs = 0;
        d = 0;
        tmp = *buf;
    }

    k = len == 64 ? ~((uint64_t)0) : (((uint64_t)1 << len) - 1) << t_len;
    tmp = (tmp & k) << ofs;
    tmp >>= t_len;
    d = (d & ~(k << ofs)) | tmp;

    b->buf[pos] = htole64(d);
    b->bitio_wp += len;

    return len + t_len;
}

int
bitio_write(struct bitio *b, uint64_t *buf, int len)
{
    int bits = sizeof(uint64_t) * 8;
    int i = len / bits;
    int r = len % bits;
    int written = 0, count = 0, ret;
    while (i > 0) {
        ret = bitio_write_64(b, &buf[count], bits);
        if (ret <= 0) {
            if (ret == 0) {
                return written;
            }
            return ret;
        }
        written += ret;
        i--;
        count++;
    }
    if (r != 0) {
        ret = bitio_write_64(b, &buf[count], r);
        if (ret <= 0) {
            if (ret == 0) {
                return written;
            }
            return ret;
        }
        written += ret;
    }
    return written;
}

int
bitio_close(struct bitio *b)
{
    if (b->bitio_mode == 'w') {
        if (b->bitio_wp % 8 != 0) {
            b->bitio_wp += 8 - (b->bitio_wp % 8);
        }

        if (bitio_flush(b) < 0 || b -> bitio_wp > 0) {
            goto fail;
        }
    }

    close(b->bitio_fd);
    free(b);
    return 0;

fail:
    close(b->bitio_fd);
    free(b);
    return -1;
}
