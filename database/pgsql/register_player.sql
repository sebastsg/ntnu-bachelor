create or replace procedure register_player (
    in in_display_name  varchar(32),
    in in_email         varchar(128),
    in in_password_hash varchar(256)
)
language plpgsql
as $$
    insert into `player` (`id`, `display_name`, `email`, `password_hash`)
         values (0, in_display_name, in_email, in_password_hash);
$$;
