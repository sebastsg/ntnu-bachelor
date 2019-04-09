<h2>Leaderboard</h2>
<table id="leaderboard">
    <tr>
        <td>Player</td>
        <td>Health</td>
        <td>Stamina</td>
        <td>Swordsmanship</td>
        <td>Shielding</td>
        <td>Axe</td>
        <td>Spear</td>
        <td>Fishing</td>
    </tr>
    <?php
    $players = find_top_players();
    foreach ($players as $name => $player) {
        echo "<tr><td>$name</td>";
        foreach ($player as $stat) {
            echo '<td>' . $stat['level'] . '</td>';
        }
        echo '</tr>';
    }
    ?>
</table>
