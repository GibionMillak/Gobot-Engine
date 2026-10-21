#pragma once

#include "core/object/class_db.h"
#include "scene/main/multiplayer_peer.h"
#include "tests/test_macros.h"

namespace TestENetMultiplayerPeer {

// Use ClassDB so this test does not depend on ENet headers outside the module include path.
TEST_CASE("[ENetMultiplayerPeer] create_server sets the host channel limit") {
	Ref<MultiplayerPeer> peer = Object::cast_to<MultiplayerPeer>(ClassDB::instantiate("ENetMultiplayerPeer"));
	REQUIRE(peer.is_valid());

	SUBCASE("Default channels") {
		REQUIRE_EQ(int(peer->call("create_server", 0, 4)), OK);
		// Zero selects ENet's maximum of 255 channels.
		Object *host = peer->call("get_host");
		CHECK_EQ(int(host->call("get_max_channels")), 255);
	}

	SUBCASE("Custom channels") {
		REQUIRE_EQ(int(peer->call("create_server", 0, 4, 3)), OK);
		// The server adds two reserved system channels to the requested count.
		Object *host = peer->call("get_host");
		CHECK_EQ(int(host->call("get_max_channels")), 5);
	}

	peer->close();
}

} // namespace TestENetMultiplayerPeer
