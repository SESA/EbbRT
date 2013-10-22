# EbbRT: Distributed, Elastic, Runtime
# Copyright (C) 2013 SESA Group, Boston University

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.

# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
EXTRA_LIBRARIES += \
	src/ebb/Network/libLWIPNetwork.a \
	src/ebb/Network/libSocketNetwork.a

src_ebb_Network_libLWIPNetwork_a_SOURCES = \
	src/ebb/Network/LWIPNetwork.cpp \
	src/ebb/Network/echo.c \
	src/ebb/Network/lwip-1.4.1/src/core/mem.c \
	src/ebb/Network/lwip-1.4.1/src/core/memp.c \
	src/ebb/Network/lwip-1.4.1/src/core/netif.c \
	src/ebb/Network/lwip-1.4.1/src/core/pbuf.c \
	src/ebb/Network/lwip-1.4.1/src/core/raw.c \
	src/ebb/Network/lwip-1.4.1/src/core/stats.c \
	src/ebb/Network/lwip-1.4.1/src/core/sys.c \
	src/ebb/Network/lwip-1.4.1/src/core/timers.c \
	src/ebb/Network/lwip-1.4.1/src/core/tcp.c \
	src/ebb/Network/lwip-1.4.1/src/core/tcp_in.c \
	src/ebb/Network/lwip-1.4.1/src/core/tcp_out.c \
	src/ebb/Network/lwip-1.4.1/src/core/udp.c \
	src/ebb/Network/lwip-1.4.1/src/core/def.c \
	src/ebb/Network/lwip-1.4.1/src/core/dhcp.c \
	src/ebb/Network/lwip-1.4.1/src/core/dns.c \
	src/ebb/Network/lwip-1.4.1/src/core/init.c \
	src/ebb/Network/lwip-1.4.1/src/core/ipv4/icmp.c \
	src/ebb/Network/lwip-1.4.1/src/core/ipv4/ip.c \
	src/ebb/Network/lwip-1.4.1/src/core/ipv4/inet.c \
	src/ebb/Network/lwip-1.4.1/src/core/ipv4/ip_addr.c \
	src/ebb/Network/lwip-1.4.1/src/core/ipv4/ip_frag.c \
	src/ebb/Network/lwip-1.4.1/src/core/ipv4/inet_chksum.c \
	src/ebb/Network/lwip-1.4.1/src/netif/etharp.c

src_ebb_Network_libLWIPNetwork_a_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-iquote $(top_srcdir)/src/ebb/Network/lwip-1.4.1/src/include \
	-iquote $(top_srcdir)/src/ebb/Network/lwip-1.4.1/src/include/ipv4 \
	-iquote $(top_srcdir)/src/ebb/Network

src_ebb_Network_libSocketNetwork_a_SOURCES = \
	src/ebb/Network/SocketNetwork.cpp
