/*
 * This file is part of the VanitySearch distribution (https://github.com/JeanLucPons/VanitySearch).
 * Copyright (c) 2019 Jean Luc PONS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

// ---------------------------------------------------------------------------------
// Bech32 (P2WPKH)
// ---------------------------------------------------------------------------------

__device__ __constant__ char *bech32_charset = (char *)"qpzry9x8gf2tvdw0s3jn54khce6mua7l";

__device__ __forceinline__ uint32_t _bech32_polymod_step(uint32_t pre) {
  uint32_t b = pre >> 25;
  return ((pre & 0x1FFFFFF) << 5)
    ^ (-(b & 1)         & 0x3b6a57b2U)
    ^ (-((b >> 1) & 1)  & 0x26508e6dU)
    ^ (-((b >> 2) & 1)  & 0x1ea119faU)
    ^ (-((b >> 3) & 1)  & 0x3d4233ddU)
    ^ (-((b >> 4) & 1)  & 0x2a1462b3U);
}

// Encode hash160 as a P2WPKH Bech32 address ("bc1q..." 42 chars).
// out must be at least 43 bytes.  hash[5] is the 20-byte hash in little-endian uint32_t.
__device__ __noinline__ void _GetBech32Address(uint32_t *hash, char *out) {

  // Checksum state after processing the fixed "bc" HRP (precomputed).
  // Derivation: start=1, polymod('b'>>5=3), polymod('c'>>5=3),
  //   polymod(sep), polymod('b'&31=2), polymod('c'&31=3) => 0x2318043
  uint32_t chk = 0x2318043U;

  out[0] = 'b'; out[1] = 'c'; out[2] = '1';

  // Witness version 0 → charset[0] = 'q'; polymod step XOR 0 = just step
  chk = _bech32_polymod_step(chk);
  out[3] = bech32_charset[0];

  // Extract 20 bytes from hash[] (little-endian uint32_t)
  uint8_t b[20];
#pragma unroll
  for (int i = 0; i < 5; i++) {
    b[4*i  ] =  hash[i]        & 0xff;
    b[4*i+1] = (hash[i] >>  8) & 0xff;
    b[4*i+2] = (hash[i] >> 16) & 0xff;
    b[4*i+3] = (hash[i] >> 24) & 0xff;
  }

  // Convert 20 bytes (160 bits) to 32 5-bit groups — no padding needed (160 % 5 == 0)
  // Process as 4 chunks of 5 bytes → 8 groups each
  uint8_t g;
  int p = 4;
#pragma unroll
  for (int i = 0; i < 4; i++) {
    int k = 5 * i;
    g = b[k] >> 3;                           chk = _bech32_polymod_step(chk) ^ g; out[p++] = bech32_charset[g];
    g = ((b[k] & 7) << 2) | (b[k+1] >> 6);  chk = _bech32_polymod_step(chk) ^ g; out[p++] = bech32_charset[g];
    g = (b[k+1] >> 1) & 31;                  chk = _bech32_polymod_step(chk) ^ g; out[p++] = bech32_charset[g];
    g = ((b[k+1] & 1) << 4) | (b[k+2] >> 4);chk = _bech32_polymod_step(chk) ^ g; out[p++] = bech32_charset[g];
    g = ((b[k+2] & 15) << 1) | (b[k+3] >> 7);chk= _bech32_polymod_step(chk) ^ g; out[p++] = bech32_charset[g];
    g = (b[k+3] >> 2) & 31;                  chk = _bech32_polymod_step(chk) ^ g; out[p++] = bech32_charset[g];
    g = ((b[k+3] & 3) << 3) | (b[k+4] >> 5);chk = _bech32_polymod_step(chk) ^ g; out[p++] = bech32_charset[g];
    g = b[k+4] & 31;                          chk = _bech32_polymod_step(chk) ^ g; out[p++] = bech32_charset[g];
  }
  // p == 36 here

  // Six finalisation rounds, then XOR 1
  for (int i = 0; i < 6; i++) chk = _bech32_polymod_step(chk);
  chk ^= 1;

  // Write 6 checksum characters then null terminator
  for (int i = 0; i < 6; i++)
    out[36 + i] = bech32_charset[(chk >> ((5 - i) * 5)) & 31];
  out[42] = '\0';
}

// ---------------------------------------------------------------------------------
// Base58
// ---------------------------------------------------------------------------------

__device__ __constant__ char *pszBase58 = (char *)"123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

__device__ __constant__ int8_t b58digits_map[] = {
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
  -1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
  22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
  -1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
  47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
};

__device__ __noinline__ void _GetAddress(int type,uint32_t *hash,char *b58Add) {

  uint32_t addBytes[16];
  uint32_t s[16];
  unsigned char A[25];
  unsigned char *addPtr = A;
  int retPos = 0;
  unsigned char digits[128];

  switch (type) {

  case BECH32:
    _GetBech32Address(hash, b58Add);
    return;

  case P2PKH:
    A[0] = 0x00;
    break;

  case P2SH:
    A[0] = 0x05;
    break;

  }
  memcpy(A + 1, (char *)hash, 20);

  // Compute checksum

  addBytes[0] = __byte_perm(hash[0], (uint32_t)A[0], 0x4012);
  addBytes[1] = __byte_perm(hash[0], hash[1], 0x3456);
  addBytes[2] = __byte_perm(hash[1], hash[2], 0x3456);
  addBytes[3] = __byte_perm(hash[2], hash[3], 0x3456);
  addBytes[4] = __byte_perm(hash[3], hash[4], 0x3456);
  addBytes[5] = __byte_perm(hash[4], 0x80, 0x3456);
  addBytes[6] = 0;
  addBytes[7] = 0;
  addBytes[8] = 0;
  addBytes[9] = 0;
  addBytes[10] = 0;
  addBytes[11] = 0;
  addBytes[12] = 0;
  addBytes[13] = 0;
  addBytes[14] = 0;
  addBytes[15] = 0xA8;

  SHA256Initialize(s);
  SHA256Transform(s, addBytes);

#pragma unroll 8
  for (int i = 0; i < 8; i++)
    addBytes[i] = s[i];

  addBytes[8] = 0x80000000;
  addBytes[9] = 0;
  addBytes[10] = 0;
  addBytes[11] = 0;
  addBytes[12] = 0;
  addBytes[13] = 0;
  addBytes[14] = 0;
  addBytes[15] = 0x100;

  SHA256Initialize(s);
  SHA256Transform(s, addBytes);

  A[21] = ((uint8_t *)s)[3];
  A[22] = ((uint8_t *)s)[2];
  A[23] = ((uint8_t *)s)[1];
  A[24] = ((uint8_t *)s)[0];

  // Base58

  // Skip leading zeroes
  while(addPtr[0] == 0) {
    b58Add[retPos++] = '1';
    addPtr++;
  }
  int length = 25-retPos;

  int digitslen = 1;
  digits[0] = 0;
  for (int i = 0; i < length; i++) {
    uint32_t carry = addPtr[i];
    for (int j = 0; j < digitslen; j++) {
      carry += (uint32_t)(digits[j]) << 8;
      digits[j] = (unsigned char)(carry % 58);
      carry /= 58;
    }
    while (carry > 0) {
      digits[digitslen++] = (unsigned char)(carry % 58);
      carry /= 58;
    }
  }

  // reverse
  for (int i = 0; i < digitslen; i++)
    b58Add[retPos++] = (pszBase58[digits[digitslen - 1 - i]]);

  b58Add[retPos] = 0;

}
