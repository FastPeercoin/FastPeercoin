FastPeercoin
============

### What is FastPeercoin?
It's Peercoin, but block times are sped way up.

FastPeercoin aims to simplify the code as much as possible.

It's a payment system first and foremost.

### Building and running

1. Use VirtualBox and Ubuntu 24.04 (Noble Numbat)

https://www.virtualbox.org

https://releases.ubuntu.com/24.04.2/ubuntu-24.04.2-desktop-amd64.iso

2. Install dependencies

```
sudo apt update
sudo apt install git build-essential autoconf pkg-config libtool
sudo apt install libboost-all-dev libssl-dev libdb++-dev
sudo apt install qtbase5-dev qttools5-dev qttools5-dev-tools 
```

3. Git clone the repository

```
git clone https://github.com/FastPeercoin/FastPeercoin
```

4. Checkout the FastPeercoin branch

```
cd FastPeercoin
git checkout FastPeercoin
```

5. Compile

```
./autogen.sh
./configure --with-incompatible-bdb
make
```

6. Run

```
./src/qt/FastPeercoin-qt
```

### What happened to mining?

CPU mining was possible for the first few weeks of the FastPeercoin project but has evolved to using ASIC devices.

If you'd like to mine, have a look at the Docker/command-line project: https://github.com/FastPeercoin/FastPeercoind

And the Stratum Server setup guide: https://talk.peercoin.net/t/guide-how-to-solo-mine-peercoin/16353
