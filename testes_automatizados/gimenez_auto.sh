#!/bin/bash

result_folder="result"
build_folder="build"
thread_number=4

flags=("-s "{10,20,30,40}" --scene "{random_spheres,two_spheres})
echo ------------- ${#flags[@]}

function bar
{
	echo =========================================================================
}

function bar2
{
	echo . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
}

function remove_all
{
	if [ -d $build_folder ] ; then
		rm -r $build_folder
	fi
	if [ -d $result_folder ] ; then
		rm -r $result_folder
	fi
}

function build
{
	mkdir $build_folder
	cd $build_folder
	cmake ../..
	make
	cd ..
}

function create_folder
{
	if [ ! -d $result_folder ] ; then
		mkdir $result_folder
	fi
}

function execute
{
	local executable=$1 quantity=$2 result_file="" result_log="" result_time="" 
	create_folder
	
	for (( qt=1 ; qt <= quantity ; qt++ )) ; 
	do
		for (( threads=1 ; threads <=  thread_number ; threads++)) ;
		do
			for (( flag_index=0 ; flag_index <  ${#flags[@]} ; flag_index++ )); 
			do
				local flag=${flags[$flag_index]}
				result_file=$result_folder/thread_${threads}_exec_${qt}_${flag_index}_image.ppm
				result_log=$result_folder/thread_${threads}_exec_${qt}_${flag_index}_log.txt
				result_time=$result_folder/thread_${threads}_exec_${qt}_${flag_index}_time.txt
				echo "realizando teste com:"
				echo "$threads thread(s)"
				echo "exec num : ${qt}.${flag_index}"
				echo "executando com: ./$executable $flag ( flag id = $flag_index )"
				time ("$(./$executable $flag > $result_file 2> $result_log)"  ) 2> $result_time
				echo execução do teste concluida
				bar
			done
		done
	done
}

function to_num
{
	local num=($(echo $1 | tr "m" "\n"))
	num[1]=${num[1]/s/}
	dc <<< "6 k ${num[0]} 60 * ${num[1]} + $2 + p"
}

function media
{
	for (( threads=1 ; threads <= thread_number ; threads++ ));
	do
		for (( flag_index=0 ; flag_index<${#flags[@]} ; flag_index++ ));
		do
			local all_test=($(ls $result_folder/thread_${threads}*_${flag_index}_time.txt))
			local total=${#all_test[@]}
			echo "Exibindo testes com ${threads} thread(s)"
			echo "executados com as seguintes flags : ${flags[$flag_index]}"
			local real="0" sys="0" user="0"
			bar2
			echo "resultado por teste"
			for (( test_case=0 ; test_case < total ; test_case ++));
			do
				result=($(cat ${all_test[$test_Case]}))
				real=$(to_num ${result[1]} $real)
				user=$(to_num ${result[3]} $user)
				sys=$(to_num ${result[5]} $sys)
				echo "REAL : ${result[1]} , USER : ${result[3]} , SYS : ${result[5]} "
			done
			bar2
		done
		echo "somas (segundos)" 
		echo "soma total REAL : $real"
		echo "soma total USER : $user"
		echo "soma total SYS  : $sys"
		bar2
		echo "medias (segundos)"
		echo "REAL : " $(dc <<< "6 k $real $total / p")
		echo "USER : " $(dc <<< "6 k $user $total / p")
		echo "SYS  : " $(dc <<< "6 k $sys $total  / p")
		bar
	done
}

bar
echo Esse script deverá ser executado a partir da pasta testes automatizados
bar
echo opções possiveis
echo ... build
echo "... execute <executavel> <quantidade_por_thread>"
echo ... remove_all 
echo ... media
bar
echo "configuraçoes atuais: (threads: $thread_number)"
bar

$1 $2 $3
