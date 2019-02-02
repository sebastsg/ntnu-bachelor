#pragma once

#include "transform.hpp"
#include "io.hpp"

class world_state;

enum class game_object_type {
	decoration,
	character,
	item_spawn,
	interactive,
	total_types
};

std::ostream& operator<<(std::ostream& out, game_object_type type);

struct game_object_definition {
	int id = -1;
	game_object_type type = game_object_type::decoration;
	std::string name;
	bool is_animated = false;
	std::string model;
};

struct character_object_definition : game_object_definition {
	
};

struct decoration_object_definition : game_object_definition {

};

class game_object_definition_list {
public:

	game_object_definition_list();

	void save(const std::string& path) const;
	void load(const std::string& path);

	game_object_definition& get(long long id);
	int count() const;

	bool add(const game_object_definition& definition);
	bool conflicts(const game_object_definition& definition) const;

private:

	std::vector<game_object_definition> definitions;
	game_object_definition invalid;

};

class game_object {
public:

	no::transform transform;

	game_object(world_state& world);

	virtual ~game_object() = default;

	virtual void update() = 0;
	virtual void write(no::io_stream& stream) const;
	virtual void read(no::io_stream& stream);

	float x() const;
	float y() const;
	float z() const;
	int id() const;
	int definition() const;

	no::vector2i tile() const;

protected:

	world_state& world;
	int definition_id = -1;
	int instance_id = -1;

};

class decoration_object : public game_object {
public:

	std::string model;
	std::string texture;

	decoration_object(world_state& world);

	void update() override;
	void write(no::io_stream& stream) const override;
	void read(no::io_stream& stream) override;

};
