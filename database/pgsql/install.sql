set default_storage_engine = InnoDB;

create table `player` (
    `id`            int unsigned not null auto_increment primary key,
    `display_name`  varchar(32)  not null unique,
    `email`         varchar(128) not null unique,
    `password_hash` varchar(256) not null,
    `created_at`    datetime     not null default current_timestamp,
    `updated_at`    datetime     not null default current_timestamp on update current_timestamp
);
