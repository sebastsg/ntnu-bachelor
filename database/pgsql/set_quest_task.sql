create or replace procedure set_quest_task (
    in in_player_id int,
    in in_quest     int,
    in in_task      int,
    in in_progress  int
)
language plpgsql
as $$
begin
    insert into quest_task (player_id, quest, task, progress)
         values (in_player_id, in_quest, in_task, in_progress)
    on conflict 
        on constraint pk_quest_task
            do update
                  set progress = in_progress;
end;
$$;
