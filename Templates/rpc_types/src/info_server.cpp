//used library
#include <iostream>
#include <string>
#include <memory>
#include <sstream>

//gRCP
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

//grcp.pb.h file
#include "../proto/info.grpc.pb.h"

//gRPC server library
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;	//For working with request
using grpc::Status;			//For working with service
using grpc::ServerWriter;  	//For stream data from server
using grpc::ServerReader;	//For stream data from client
using grpc::ServerReaderWriter;	//When stream from both client/server

//all messages and services from proto 
//package - message
//package - service
using infowriter::InfoSearcher;
using infowriter::Numeral;
using infowriter::TextNumeral;
using infowriter::NumberInfo;
using infowriter::Text;

//Implementation of service
//class service_name+"Service"+"Impl" final : public service_name::"Service"
class InfoSearcherServiceImpl final : public InfoSearcher::Service {
    //unary(simple) rpc
    //code for rpc NumeralConverter
    Status NumeralConverter(ServerContext* context,         //context object for rpc
                            const Numeral* numeralRequest,  //get from client data
                            TextNumeral *textNumeralBuffer) //buffer for responce
                            override{
        
        //Convert taked numeral to text
        switch (numeralRequest->num()){
            case 0:
                textNumeralBuffer->set_textnum("Zero");
                break;
            case 1:
                textNumeralBuffer->set_textnum("One");
                break;
            case 2:
                textNumeralBuffer->set_textnum("Two");
                break;
            case 3:
                textNumeralBuffer->set_textnum("Three");
                break;
            case 4:
                textNumeralBuffer->set_textnum("Four");
                break;
            case 5:
                textNumeralBuffer->set_textnum("Five");
                break;
            case 6:
                textNumeralBuffer->set_textnum("Six");
                break;
            case 7:
                textNumeralBuffer->set_textnum("Seven");
                break;
            case 8:
                textNumeralBuffer->set_textnum("Eight");
                break;
            case 9:
                textNumeralBuffer->set_textnum("Nine");
                break;
            default:
                textNumeralBuffer->set_textnum("It's not numeral");
                break;
        }
        return Status::OK;
    }
	
	//stream from server
	Status SimpleNumbersInRange(ServerContext* context,         //context object for rpc
								const Numeral* numeralLimit,  	//get from client data
								ServerWriter<Numeral>* writer	//stream of some data to return on client
								)override{
		for (int i=2; i<=numeralLimit->num();i++){
			int amount = 0;
			for (int j=2; j<=i/2; j++){
				if (i%j==0) amount++;
			}			
			if(amount==0){
				Numeral n;
				n.set_num(i);
				writer->Write(n);
			} 
		}
		
		return Status::OK;
	}
	
	//stream from client
	Status CalculateSimpleNumber(ServerContext* context,         	//context object for rpc
								ServerReader<Numeral>* reader,  	//get from client stream of numbers
								Numeral* amountSimple				//buffer for responce - amount of simple numbers
								)override{
		Numeral oneNumberFromClient;
		
		while (reader->Read(&oneNumberFromClient)){					//Read on number from stream
			for(int i=2; i<oneNumberFromClient.num(); i++){
				if(oneNumberFromClient.num()%i==0) break;
				if (i==oneNumberFromClient.num()-1) amountSimple->set_num(amountSimple->num()+1);
			}
		}
		
		return Status::OK;
	}
	
	//stream from both client/server
	Status RandomNumberAnalize(ServerContext* context,         									//context object for rpc
								ServerReaderWriter<NumberInfo, Numeral>* stream) override {  	//get from client stream of numbers and return analisis for each
								
		Numeral oneNumberForAnalis;
			
		//Read from client's stream
		while (stream->Read(&oneNumberForAnalis)){
			NumberInfo currentNumberInfo;
			std::unique_lock<std::mutex> lock(mut);		//lock thread for analis current number
			
			//Find amount numerals in number
			std::stringstream ss;
			ss << oneNumberForAnalis.num();
			currentNumberInfo.set_numamount((ss.str()).size());
			
			//check if number even or not
			if (oneNumberForAnalis.num()%2==0){
				currentNumberInfo.set_even(true);
			}
			else{
				currentNumberInfo.set_even(false);
			}
			
			//create array from number numerals
			int num = oneNumberForAnalis.num();
			std::vector<int> vec;
			for(int i=0; i<currentNumberInfo.numamount(); i++){
				vec.insert(vec.begin(), num%10);
				num/=10;
			}
			for(const auto& v:vec){
				currentNumberInfo.add_numarr(v);
			}
			
			stream->Write(currentNumberInfo);	//Send to client responce with number info
		}
		
		return Status::OK;
	}
	
	//Chat just for fun
	Status Chat(ServerContext* context,        
                            const Text* textRequest,  	//get from client data
                            Text *textResponce) //buffer for responce
                            override{
        
		std::cout<<"Someone: "<<textRequest->text()<<std::endl;
		
		std::string str;
		std::cout<<"You: ";
		getline(std::cin, str);
		
		textResponce->set_text(str);		
        return Status::OK;
    }

private:
	std::mutex mut;
};



void RunServer() {
    std::string server_address("192.168.1.109:25565");          //address and port for listening
    InfoSearcherServiceImpl service;                  //Instance of InfoSearcher service

    //Instance of gRPC server builder class
    //Containe port for listening and instance of service
    ServerBuilder builder;    

    //address and port for listening
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    //Registration Instance of created service in builder
    builder.RegisterService(&service);

    //Start server and save it in smart pointer RAII
    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "Server listening on " << server_address << std::endl;
    
    //Wait for Shutdown or for connection with client
    server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}