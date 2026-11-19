/*---------------------------------------------------------------------------*/
/* SfxAuthor.cpp                                                             */
/*                                                                           */
/* Deterministic code generator for the 'SfxAuthor' configuration value.     */
/*                                                                           */
/* Code format:  XXXXX-XXXXX-XXXXX-XXXXX-XXXXX                               */
/*   - SHA-256 over the UTF-16LE bytes of the author string                  */
/*   - key = digest[0..15] XOR digest[16..31]                                */
/*   - value i = (key[(i*5)/8] >> ((i*5)%8)) & 31                            */
/*   - alphabet: 123456789ABCDEFGHJKMNPQRSTUVWXYZ (32 chars, no 0/I/L/O)     */
/*                                                                           */
/* Self-contained: no module headers, can be compiled standalone.            */
/*---------------------------------------------------------------------------*/

static const wchar_t *g_AuthorAlphabet = L"123456789ABCDEFGHJKMNPQRSTUVWXYZ";

static const unsigned int g_Sha256K[64] =
{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static unsigned int Sha256Ror( unsigned int x, int n )
{
    return (x >> n) | (x << (32 - n));
}

static void Sha256Block( unsigned int h[8], const unsigned char *p )
{
    unsigned int w[64];
    unsigned int a, b, c, d, e, f, g, hh;
    int i;

    for( i = 0; i < 16; i++ )
    {
        w[i] = ((unsigned int)p[i*4] << 24)
             | ((unsigned int)p[i*4+1] << 16)
             | ((unsigned int)p[i*4+2] << 8)
             | (unsigned int)p[i*4+3];
    }
    for( i = 16; i < 64; i++ )
    {
        unsigned int s0 = Sha256Ror(w[i-15], 7) ^ Sha256Ror(w[i-15], 18) ^ (w[i-15] >> 3);
        unsigned int s1 = Sha256Ror(w[i-2], 17) ^ Sha256Ror(w[i-2], 19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    a = h[0]; b = h[1]; c = h[2]; d = h[3];
    e = h[4]; f = h[5]; g = h[6]; hh = h[7];

    for( i = 0; i < 64; i++ )
    {
        unsigned int S1 = Sha256Ror(e, 6) ^ Sha256Ror(e, 11) ^ Sha256Ror(e, 25);
        unsigned int ch = (e & f) ^ ((~e) & g);
        unsigned int t1 = hh + S1 + ch + g_Sha256K[i] + w[i];
        unsigned int S0 = Sha256Ror(a, 2) ^ Sha256Ror(a, 13) ^ Sha256Ror(a, 22);
        unsigned int maj = (a & b) ^ (a & c) ^ (b & c);
        unsigned int t2 = S0 + maj;

        hh = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

struct CSha256Ctx
{
    unsigned int h[8];
    unsigned long long total;
    unsigned char buf[64];
    unsigned int buflen;
};

static void Sha256Init( CSha256Ctx *ctx )
{
    ctx->h[0] = 0x6a09e667; ctx->h[1] = 0xbb67ae85;
    ctx->h[2] = 0x3c6ef372; ctx->h[3] = 0xa54ff53a;
    ctx->h[4] = 0x510e527f; ctx->h[5] = 0x9b05688c;
    ctx->h[6] = 0x1f83d9ab; ctx->h[7] = 0x5be0cd19;
    ctx->total = 0;
    ctx->buflen = 0;
}

static void Sha256Update( CSha256Ctx *ctx, const unsigned char *data, unsigned int size )
{
    unsigned int i = 0;
    ctx->total += size;

    if( ctx->buflen > 0 )
    {
        while( i < size && ctx->buflen < 64 )
            ctx->buf[ctx->buflen++] = data[i++];
        if( ctx->buflen == 64 )
        {
            Sha256Block(ctx->h, ctx->buf);
            ctx->buflen = 0;
        }
    }
    while( size - i >= 64 )
    {
        Sha256Block(ctx->h, data + i);
        i += 64;
    }
    while( i < size )
        ctx->buf[ctx->buflen++] = data[i++];
}

static void Sha256Final( CSha256Ctx *ctx, unsigned char digest[32] )
{
    unsigned long long bits = ctx->total * 8;
    unsigned char pad[72];
    unsigned int padLen;
    int i;

    padLen = (ctx->buflen < 56) ? (56 - ctx->buflen) : (120 - ctx->buflen);
    pad[0] = 0x80;
    for( i = 1; i < (int)padLen; i++ )
        pad[i] = 0;
    Sha256Update(ctx, pad, padLen);

    {
        unsigned char lenBuf[8];
        for( i = 0; i < 8; i++ )
            lenBuf[i] = (unsigned char)(bits >> (56 - i*8));
        Sha256Update(ctx, lenBuf, 8);
    }

    for( i = 0; i < 8; i++ )
    {
        digest[i*4]   = (unsigned char)(ctx->h[i] >> 24);
        digest[i*4+1] = (unsigned char)(ctx->h[i] >> 16);
        digest[i*4+2] = (unsigned char)(ctx->h[i] >> 8);
        digest[i*4+3] = (unsigned char)(ctx->h[i]);
    }
}

/*---------------------------------------------------------------------------*/
/* Public entry                                                              */
/*---------------------------------------------------------------------------*/

extern "C" void GetSfxAuthorCode( const wchar_t *lpwszAuthor, wchar_t *lpwszCode, int cchCode )
{
    CSha256Ctx ctx;
    unsigned char digest[32];
    unsigned char key[16];
    unsigned int len = 0;
    unsigned int i;
    int pos = 0;

    if( lpwszCode == 0 || cchCode <= 0 )
        return;
    lpwszCode[0] = 0;
    if( cchCode < 30 )
        return;

    if( lpwszAuthor != 0 )
    {
        while( lpwszAuthor[len] != 0 )
            len++;
    }

    Sha256Init(&ctx);
    for( i = 0; i < len; i++ )
    {
        unsigned short c = (unsigned short)lpwszAuthor[i];
        unsigned char pair[2];
        pair[0] = (unsigned char)(c & 0xFF);
        pair[1] = (unsigned char)((c >> 8) & 0xFF);
        Sha256Update(&ctx, pair, 2);
    }
    Sha256Final(&ctx, digest);

    /* key = first 16 bytes XOR second 16 bytes (as in the original module) */
    for( i = 0; i < 16; i++ )
        key[i] = digest[i] ^ digest[i + 16];

    /* 5-bit value is taken from a single byte: key[bit/8] >> (bit%8) */
    for( i = 0; i < 25; i++ )
    {
        unsigned int bit = i * 5;
        unsigned int value = (key[bit >> 3] >> (bit & 7)) & 31;
        if( i > 0 && (i % 5) == 0 )
            lpwszCode[pos++] = L'-';
        lpwszCode[pos++] = g_AuthorAlphabet[value];
    }
    lpwszCode[pos] = 0;
}
