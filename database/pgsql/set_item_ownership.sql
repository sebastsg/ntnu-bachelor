create or replace procedure set_item_ownership (
    in in_player_id int,
    in in_container int,
    in in_slot      int,
    in in_item      int,
    in in_stack     int
)
language plpgsql
as $$
begin
    insert into item_ownership (player_id, container, slot, item, stack)
         values (in_player_id, in_container, in_slot, in_item, in_stack)
    on conflict 
        on constraint pk_item_ownership
            do update
                set item = in_item,
                    stack = in_stack;
end;
$$;
