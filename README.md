# VanitySearch

VanitySearch is a Bitcoin address prefix finder. If you want to generate safe private keys, use the `-s` option to enter your passphrase which will be used for generating a base key as per the BIP38 standard (*VanitySearch -s "My PassPhrase" 1MyPrefix*). You can also use *VanitySearch -ps "My PassPhrase"* which will add a cryptographically secure seed to your passphrase.

VanitySearch may not compute a good grid size for your GPU, so try different values using the `-g` option in order to get the best performance. If you want to use GPUs and CPUs together, you may get best performance by keeping one CPU core for handling GPU/CPU exchanges (use `-t` to set the number of CPU threads).

# Features

<ul>
  <li>Fixed size arithmetic</li>
  <li>Fast modular inversion (Delayed Right Shift 62 bits)</li>
  <li>secp256k1 fast modular multiplication (2-step folding 512 bits to 256 bits using 64-bit digits)</li>
  <li>Uses elliptic curve properties to generate multiple keys per step</li>
  <li>SSE Secure Hash Algorithm SHA256 and RIPEMD160 (CPU)</li>
  <li>Multi-GPU support</li>
  <li>CUDA optimisation via inline PTX assembly</li>
  <li>Seed protected by pbkdf2_hmac_sha512 (BIP38)</li>
  <li>Support P2PKH, P2SH and Bech32 (P2WPKH) addresses</li>
  <li>GPU wildcard pattern matching for all address types (P2PKH, P2SH, Bech32)</li>
  <li>Support split-key vanity address</li>
</ul>

# Discussion Thread

[Discussion about VanitySearch @ bitcointalk](https://bitcointalk.org/index.php?topic=5112311.0)

# Usage

```
VanitySearch [-check] [-v] [-u] [-b] [-c] [-gpu] [-stop] [-i inputfile]
             [-gpuId gpuId1[,gpuId2,...]] [-g g1x,g1y,[,g2x,g2y,...]]
             [-o outputfile] [-m maxFound] [-ps seed] [-s seed] [-t nbThread]
             [-nosse] [-r rekey] [-check] [-kp] [-sp startPubKey]
             [-rp privkey partialkeyfile] [prefix]

 prefix: prefix to search (may contain wildcards '?' or '*')
 -v: Print version
 -u: Search uncompressed addresses
 -b: Search both uncompressed and compressed addresses
 -c: Case insensitive search
 -gpu: Enable GPU calculation
 -stop: Stop when all prefixes are found
 -i inputfile: Get list of prefixes to search from specified file
 -o outputfile: Output results to the specified file
 -gpu gpuId1,gpuId2,...: List of GPU(s) to use, default is 0
 -g g1x,g1y,g2x,g2y,...: Specify GPU(s) kernel grid size, default is 8*(MP number),128
 -m: Specify maximum number of prefixes found by each kernel call
 -s seed: Specify a seed for the base key, default is random
 -ps seed: Specify a seed concatenated with a cryptographically secure random seed
 -t threadNumber: Specify number of CPU threads, default is number of cores
 -nosse: Disable SSE hash function
 -l: List CUDA enabled devices
 -check: Check CPU and GPU kernel output against CPU reference
 -cp privKey: Compute public key (privKey in hex format)
 -kp: Generate key pair
 -rp privkey partialkeyfile: Reconstruct final private key(s) from partial key(s) info
 -sp startPubKey: Start the search with a public key (for private key splitting)
 -r rekey: Rekey interval in MegaKeys, default is disabled
```

Wildcard patterns can be used with all address types and work on both CPU and GPU:

```
./VanitySearch -stop -gpu "1*love*"
./VanitySearch -stop -gpu "3*coin*"
./VanitySearch -stop -gpu "bc1q*cafe*"
```

Example (Linux, AMD Ryzen 9, GeForce RTX 3070):

```
$ ./VanitySearch -stop -gpu 1TryMe
VanitySearch v1.19
Difficulty: 15318045009
Search: 1TryMe [Compressed]
Start Wed Jun 17 14:00:00 2026
Base Key: DA12E013325F12D6B68520E327847218128B788E6A9F2247BC104A0EE2818F44
Number of CPU thread: 16
GPU: GPU #0 NVIDIA GeForce RTX 3070 (46x128 cores) Grid(368x128)
[1074.88 Mkey/s][GPU 1060.58 Mkey/s][Total 2^32.59][Found 1]
PubAddress: 1TryMeJT7cfs4M6csEyhWVQJPAPmJ4NGw
Priv (WIF): p2pkh:Kxs4iWcqYHGBfzVpH4K94STNMHHz72DjaCuNdZeM5VMiP9zxMg15
Priv (HEX): 0x310DBFD6AAB6A63FC71CAB1150A0305ECABBE46819641D2594155CD41D081AF1
```

```
$ ./VanitySearch -stop -gpu 3MyCoin
VanitySearch v1.19
Search: 3MyCoin [Compressed]
Start Wed Jun 17 14:01:00 2026
Number of CPU thread: 16
GPU: GPU #0 NVIDIA GeForce RTX 3070 (46x128 cores) Grid(368x128)
[1074.88 Mkey/s][GPU 1060.58 Mkey/s][Total 2^33.18][Found 1]
PubAddress: 3MyCoinoA167kmgPprAidSvv5NoM3Nh6N3
Priv (WIF): p2wpkh-p2sh:L2qvghanHHov914THEzDMTpAyoRmxo7Rh85FLE9oKwYUrycWqudp
Priv (HEX): 0xA7D14FBF43696CA0B3DBFFD0AB7C9ED740FE338B2B856E09F2E681543A444D58
```

```
$ ./VanitySearch -stop -gpu bc1quantum
VanitySearch v1.19
Search: bc1quantum [Compressed]
Start Wed Jun 17 14:02:00 2026
Number of CPU thread: 16
GPU: GPU #0 NVIDIA GeForce RTX 3070 (46x128 cores) Grid(368x128)
[1074.88 Mkey/s][GPU 1060.58 Mkey/s][Total 2^28.94][Found 1]
PubAddress: bc1quantum898l8mx5pkvq2x250kkqsj7enpx3u4yt
Priv (WIF): p2wpkh:L37xBVcFGeAZ9Tii7igqXBWmfiBhiwwiKQmchNXPV2LNREXQDLCp
Priv (HEX): 0xB00FD8CDA85B11D4744C09E65C527D35E2B1D19095CFCA0BF2E48186F31979C2
```

# Generate a vanity address for a third party using split-key

It is possible to generate a vanity address for a third party in a safe manner using split-key.
For instance, Alice wants a nice prefix but does not have CPU power. Bob has the requested CPU power but cannot know the private key of Alice, so Alice uses a split-key.

## Step 1

Alice generates a key pair on her computer then sends the generated public key and the wanted prefix to Bob. This can be done by email — nothing is secret. Alice must keep her private key safe and not expose it.
```
VanitySearch -s "AliceSeed" -kp
Priv : L4U2Ca2wyo721n7j9nXM9oUWLzCj19nKtLeJuTXZP3AohW9wVgrH
Pub  : 03FC71AE1E88F143E8B05326FC9A83F4DAB93EA88FFEACD37465ED843FCC75AA81
```
Note: The key pair is a standard secp256k1 key pair and can be generated with third-party software.

## Step 2

Bob runs VanitySearch using Alice's public key and the wanted prefix.
```
VanitySearch -sp 03FC71AE1E88F143E8B05326FC9A83F4DAB93EA88FFEACD37465ED843FCC75AA81 -gpu -stop -o keyinfo.txt 1ALice
```
This generates a keyinfo.txt file containing the partial private key.
```
PubAddress: 1ALicegohz9YgrLLa4ADCmam7X2Zr6xJZx
PartialPriv: L2hbovuDd8nG4nxjDq1yd5qDsSQiG8xFsAFbHMcThqfjSP6WLg89
```
Bob sends this file back to Alice. This can also be done by email. The partial private key does not allow anyone to guess Alice's final private key.

## Step 3

Alice reconstructs the final private key using her private key (generated in step 1) and the keyinfo.txt from Bob.

```
VanitySearch -rp L4U2Ca2wyo721n7j9nXM9oUWLzCj19nKtLeJuTXZP3AohW9wVgrH keyinfo.txt

PubAddress: 1ALicegohz9YgrLLa4ADCmam7X2Zr6xJZx
Priv (WIF): p2pkh:L1NHFgT826hYNpNN2qd85S7F7cyZTEJ4QQeEinsCFzknt3nj9gqg
Priv (HEX): 0x7BC226A19A1E9770D3B0584FF2CF89E5D43F0DC19076A7DE1943F284DA3FB2D0
```

## How it works

The `-sp` (start public key) option adds the specified starting public key (Q) to the starting keys of each thread. This means that when searching with `-sp`, you search for addr(k<sub>part</sub>.G+Q) rather than addr(k.G), where k is the private key in the first case and k<sub>part</sub> the partial private key in the second. G is the secp256k1 generator point.

The requester reconstructs the final private key as k<sub>part</sub>+k<sub>secret</sub> (mod n), where k<sub>part</sub> is the partial private key found by the searcher and k<sub>secret</sub> is the private key of Q (Q=k<sub>secret</sub>.G). This is the purpose of the `-rp` option.

The searcher found a match for addr(k<sub>part</sub>.G+k<sub>secret</sub>.G) without knowing k<sub>secret</sub>, so the requester has the wanted address and the corresponding private key k<sub>part</sub>+k<sub>secret</sub> (mod n). The searcher cannot guess this final private key because they do not know k<sub>secret</sub> (they know only Q).

Note: This explanation is simplified — it does not account for symmetry and endomorphism optimizations, but the idea is the same.

# Trying to attack a list of addresses

A Bitcoin address (P2PKH) consists of a hash160 displayed in Base58 format, meaning there are 2<sup>160</sup> possible addresses. A secure hash function can be seen as a pseudo-random number generator — it transforms a given message into a number uniformly distributed in the range [0, 2<sup>160</sup>]. The probability of hitting a particular number after n tries is 1-(1-1/2<sup>160</sup>)<sup>n</sup>.

If we have a list of m distinct addresses (m≤2<sup>160</sup>), the search space is reduced to 2<sup>160</sup>/m, the probability of finding a collision after 1 try becomes m/2<sup>160</sup>, and after n tries: 1-(1-m/2<sup>160</sup>)<sup>n</sup>.

Example: with hardware capable of generating **1 GKey/s** and an input list of **10<sup>6</sup>** addresses:

| Time     |  Probability  |
|----------|:-------------:|
| 1 second |6.8e-34|
| 1 minute |4e-32|
| 1 hour |2.4e-30|
| 1 day |5.9e-29|
| 1 year |2.1e-26|
| 10 years | 2.1e-25 |
| 1000 years | 2.1e-23 |
| Age of earth | 8.64e-17 |
| Age of universe | 2.8e-16 (much less than winning the lottery) |

Calculation done using this [online high precision calculator](https://keisan.casio.com/calculator).

Even with competitive hardware it is very unlikely that you find a collision. The birthday paradox does not apply here — it works only if the public key (not the address, but the hash of the public key) is already known. This program searches only for collisions with addresses that have a certain prefix.

# Compilation

## Linux

Install CUDA SDK (tested with CUDA 13.2). No secondary compiler is required — modern GCC (tested with GCC 15.2) works directly with recent CUDA releases.

To build the CPU-only version:
```sh
$ make all
```

To build with CUDA support, set `CCAP` to your GPU's compute capability (digits only, no dot):
```sh
$ make gpu=1 CCAP=86 all
```

Common compute capabilities: `75` (Turing/RTX 20xx), `86` (Ampere/RTX 30xx), `89` (Ada/RTX 40xx). See the [NVIDIA CUDA GPU list](https://developer.nvidia.com/cuda-gpus) for your hardware.

If CUDA is not installed in `/usr/local/cuda`, pass the path explicitly:
```sh
$ make gpu=1 CCAP=86 CUDA=/path/to/cuda all
```

Running VanitySearch (AMD Ryzen 9, 16 cores, GeForce RTX 3070):
```sh
$ ./VanitySearch -t 7 -gpu 1TryMe
VanitySearch v1.19
Difficulty: 15318045009
Search: 1TryMe [Compressed]
Number of CPU thread: 7
GPU: GPU #0 NVIDIA GeForce RTX 3070 (46x128 cores) Grid(368x128)
[1074.88 Mkey/s][GPU 1060.58 Mkey/s][Total 2^32.59][Found 1]
PubAddress: 1TryMeJT7cfs4M6csEyhWVQJPAPmJ4NGw
Priv (WIF): p2pkh:Kxs4iWcqYHGBfzVpH4K94STNMHHz72DjaCuNdZeM5VMiP9zxMg15
Priv (HEX): 0x310DBFD6AAB6A63FC71CAB1150A0305ECABBE46819641D2594155CD41D081AF1
```

## Windows

Install CUDA SDK and open `VanitySearch.sln` in Visual Studio 2017 or later. You may need to reset the *Windows SDK version* in project properties. In Build → Configuration Manager, select the *Release* configuration. Build and enjoy.

## Docker

> **Note:** The Docker images are currently unmaintained. The Dockerfiles are provided as a reference but may require updates for recent CUDA and GPU architectures.

[![Docker Stars](https://img.shields.io/docker/stars/ratijas/vanitysearch.svg)](https://hub.docker.com/r/ratijas/vanitysearch)
[![Docker Pulls](https://img.shields.io/docker/pulls/ratijas/vanitysearch.svg)](https://hub.docker.com/r/ratijas/vanitysearch)

### Supported tags (unmaintained)

 * [`latest`, `cuda-ccap-6`, `cuda-ccap-6.0` *(cuda/Dockerfile)*](./docker/cuda/Dockerfile)
 * [`cuda-ccap-5`, `cuda-ccap-5.2` *(cuda/Dockerfile)*](./docker/cuda/Dockerfile)
 * [`cuda-ccap-2`, `cuda-ccap-2.0` *(cuda/ccap-2.0.Dockerfile)*](./docker/cuda/ccap-2.0.Dockerfile)
 * [`cpu` *(cpu/Dockerfile)*](./docker/cpu/Dockerfile)

### Docker build

Docker images are built for CPU-only and for each supported CUDA compute capability (`CCAP`). Choose the latest `CCAP` supported by your hardware. A compatibility table can be found on [Wikipedia](https://en.wikipedia.org/wiki/CUDA#GPUs_supported) or at the official NVIDIA product page.

#### CPU-only
```sh
$ ./docker/cpu/build.sh
```

#### GPU
```sh
$ env CCAP=52 CUDA=10.2 ./docker/cuda/build.sh
```

# License

VanitySearch is licensed under GPLv3.
