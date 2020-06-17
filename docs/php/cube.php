<?php

    $descriptorspec = array(
        0 => array("pipe", "r"),  // stdin is a pipe that the child will read from
        1 => array("pipe", "w"),  // stdout is a pipe that the child will write to
        2 => array("pipe", "w")   // stderr is a pipe that the child will write to
    );
 
    $pwd = '/tmp';
    $process = proc_open('/usr/local/bin/cube', $descriptorspec, $pipes, $pwd);

    if (is_resource($process)) 
    {
        // sleep(1);
        fwrite($pipes[0], 'ls();');
        fclose($pipes[0]);

        // sleep(1);

        print stream_get_contents($pipes[1]);
        fclose($pipes[1]);

        // echo stream_get_contents($pipes[2]);
        fclose($pipes[2]);

        // It is important that you close any pipes before calling
        // proc_close in order to avoid a deadlock
        $return_value = proc_close($process);

        echo "command returned $return_value\n";
    }



?>