drop table if exists player;
create table player (
    id            serial       not null primary key,
    display_name  varchar(32)  not null unique,
    email         varchar(128) not null unique,
    password_hash varchar(256) not null,
    created_at    timestamp    not null default current_timestamp
);
