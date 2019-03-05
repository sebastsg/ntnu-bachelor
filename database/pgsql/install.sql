drop table if exists item_ownership;
drop table if exists variable;
drop table if exists quest_task;
drop table if exists player;
drop table if exists account;

create table account (
    email         varchar(128) not null primary key,
    password_hash varchar(256) not null,
    created_at    timestamp    not null default current_timestamp
);

create table player (
    id            serial       not null primary key,
    account_email varchar(128) not null,
    display_name  varchar(32)  not null unique,
    tile_x        int          not null default 0,
    tile_z        int          not null default 0,
    created_at    timestamp    not null default current_timestamp,
    constraint fk_player_account_email foreign key (account_email) references account (email)
);

create table item_ownership (
    player_id int not null,
    container int not null,
    slot      int not null,
    item      int not null,
    stack     int not null,
    constraint pk_item_ownership           primary key (player_id, container, slot),
    constraint fk_item_ownership_player_id foreign key (player_id) references player (id)
);

create table variable (
    player_id  int          not null,
    scope      int          not null,
    var_name   varchar(32)  not null,
    var_value  varchar(64)  not null,
    var_type   int          not null,
    created_at timestamp    not null default current_timestamp,
    constraint pk_variable           primary key (player_id, scope, var_name),
    constraint fk_variable_player_id foreign key (player_id) references player (id)
);

create quest_task (
    player_id  int       not null,
    quest      int       not null,
    task       int       not null,
    progress   int       not null,
    created_at timestamp not null default current_timestamp,
    constraint pk_quest_task           primary key (player_id, quest, task),
    constraint fk_quest_task_player_id foreign key (player_id) references player (id)
);
