# Build-Umgebung: devkitARM + makerom + bannertool
FROM devkitpro/devkitarm
RUN git config --global url."https://github.com/".insteadOf git://github.com/ \
 && git clone --depth 1 https://github.com/3DSGuy/Project_CTR.git /tmp/ctr \
 && make -C /tmp/ctr/makerom -j4 >/dev/null && cp /tmp/ctr/makerom/bin/makerom /usr/local/bin/ \
 && git clone --depth 1 --recurse-submodules https://github.com/Epicpkmn11/bannertool.git /tmp/bt \
 && make -C /tmp/bt -j4 >/dev/null && cp /tmp/bt/output/linux-x86_64/bannertool /usr/local/bin/ \
 && rm -rf /tmp/ctr /tmp/bt
