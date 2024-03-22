/*

Class:
    Passenger
        Destination
        Weight
        Start

Init()
    string state = OFFLINE
    int current_floor = 0
    int load = 0

    int num_passengers = 0
    int num_waiting = 0
    int num_serviced = 0

    List [6] Floor  (Array of 6 floors which each contains a list of passengers waiting)
    List Elevator (List that contains passengers in elevator)

    bool stop = false
    int current_destination (the floor that elevator is heading to) = 0

main()
    while(true){
        while(state != OFFLINE){
            if(num waiting > 0){
                moveElevator()
            }
            else{
                State = IDLE
            }
        }
    }

moveElevator{
    if(current_destination == 0){
        getNewDestination()
    }

    if(current_floor < current_destination){
        goUp()
    }
    else if(current_floor > current_destination){
        doDown()
    }
    
    check_floor()

    if(current_floor == current_destination){
        if(num_waiting > 0){
            getNewDestination()
        }
        else
            State = IDLE
    }


}

goUp(){
    state = UP
    if(current_floor < 6)
        current_floor += 1
}

goDown(){
    state = DOWN
    current_floor -= 1
}

check_floor(int floor_num){
    loop through elevator list:
        if(passenger.destination == floor_num)
            dropOff(passenger);
    
    loop through floor[floor_num] list:
        if(!stop && current_load + passenger.weight <= MAX_LOAD)
            load(passenger);
}

getNewDestination(){
    Loop through floors:
        if floor is not empty:
            current_destination = floor_num;
            break;
}

dropOff(passenger){
    STATE = LOADING;
    num_passengers --;
    num_serviced ++;
    current_load -= passenger.weight;
    delete passenger from elevator list
}

load(passenger){
    STATE = LOADING;
    num_waiting --;
    num_passengers ++;
    current_load += passenger.weight
    delete passenger from floor list
    add passenger to elevator list
}

write_to_proc(){
    generate text and then write to proc

    char star[6];
    for(i = 0; i<5; i++)
        if(i == current_floor)
            star[i] = '*'
        else
            star[i] = ' '
    
    string f[6]
    string waiting[6]

    loop through each floor:
        sum = 0
        loop through each item in list:
            f[i].append(passenger)
            sum ++
        
        waiting[i] = sum

    text = f" 
    Elevator State :{state} \n
    Current Floor : {current_floor} \n
    Current Load : {load}

    [{star[5]}] Floor 6 : {waiting[5]} {f[5]} 
    [{star[4]}] Floor 5 : {waiting[4]} {f[4]} 
    [{star[3]}] Floor 4 : {waiting[3]} {f[3]} 
    [{star[2]}] Floor 3 : {waiting[2]} {f[2]} 
    [{star[1]}] Floor 2 : {waiting[1]} {f[1]} 
    [{star[0]}] Floor 1 : {waiting[0]} {f[0]} 

    Number of passengers : {num_passengers}
    Number of passengers waiting : {num_waiting}
    Number of passengers serviced : {num_serviced}
    "
    
    write text to procfile

}

*/