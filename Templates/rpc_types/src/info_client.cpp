//used library
#include <iostream>
#include <string>
#include <memory>

//gRCP
#include <grpcpp/grpcpp.h>

//grcp.pb.h file
#include "../proto/info.grpc.pb.h"

//gRCP client library
using grpc::Channel;			//For create chanel with server
using grpc::ClientContext;		//Build data for sending to server
using grpc::Status;				//For server responce
using grpc::ClientReader;		//When server send stream, client must take it
using grpc::ClientWriter;		//When client send stream, server must take it
using grpc::ClientReaderWriter;	//When stream from both client/server

//all messages and services from proto 
//package - message
//package - service
using infowriter::InfoSearcher;
using infowriter::Numeral;
using infowriter::TextNumeral;
using infowriter::NumberInfo;
using infowriter::Text;

//class working with services
class InfoSearcherClient{
public:
    //constructor
    InfoSearcherClient(std::shared_ptr<Channel> channel)            //address and port for connection
                        :stub_(InfoSearcher::NewStub(channel)){}      //stub for service (set it name there)

    //unary(simple) rpc
    //code for rpc NumeralConverter
    void NumeralConverter(int numForSend){
        Numeral numeral;            //numeral for convert
        TextNumeral textNumeral;    //numeral convert to text
        
        numeral.set_num(numForSend);

        //private function in this class that send data to server
        MakeOneNumeralConversation(numeral, &textNumeral);
    }
	
	//rpc with stream from server
	void SimpleNumbersInRange(int limit){
		Numeral numeralLim;			//For send data
		Numeral simpleNum;			//For server responce
		ClientContext context;

		numeralLim.set_num(limit);
		
		std::unique_ptr<ClientReader<Numeral>> reader(							//create ClientReader to get stream from server
							stub_->SimpleNumbersInRange(&context, numeralLim));	//call SimpleNumbersInRange rpc
		
		int i=1;
		while(reader->Read(&simpleNum)){	//Read server responce while stream is not end			
			std::cout<<i++<<": "<<simpleNum.num()<<std::endl;			
		}
		
		Status status = reader->Finish();	//Check status result
		if (status.ok()) {
			std::cout << "Server stream rpc succeeded" << std::endl;
		} 
		else {
			std::cout << "Server stream rpc failed" << std::endl;
		}
	}
	
	//rpc with stream from client
	void CalculateSimpleNumber(int numAmount){
		ClientContext context;
		Numeral numForCheck;		//one number for checking
		Numeral amountOfSimple;		//Server responce
		
		std::unique_ptr<ClientWriter<Numeral>> writer(							//Send stream of data to server
						stub_->CalculateSimpleNumber(&context, &amountOfSimple));	
		
		std::cout<<"Number for check: ";
		for (int i=0; i<numAmount; i++){
			numForCheck.set_num(rand()%1001);
			std::cout<<numForCheck.num()<<" ";
			
			 if (!writer->Write(numForCheck)) {	//Send data to server
				// Broken stream
				std::cout<<"Problem with sending stream to server"<<std::endl;
				break;
			}
		}
		std::cout<<std::endl;
		
		writer->WritesDone();	//Send iformation about end of output stream
		
		Status status = writer->Finish();
		if (status.ok()) {
			std::cout << "Client stream rpc succeeded" << std::endl;
			std::cout << amountOfSimple.num() << std::endl;
		} 
		else {
			std::cout << "Client stream rpc failed" << std::endl;
		}
	}
	
	//rpc with stream from both client/server
	void RandomNumberAnalize(int numAmount){
		ClientContext context;
		
		 std::shared_ptr<ClientReaderWriter<Numeral, NumberInfo>> stream(
													stub_->RandomNumberAnalize(&context));	//create stub
													
		//create thread for client/server conversation
		std::thread writer([stream, numAmount](){			
			//send data to server
			Numeral numForAnalize;				//one number for analize
			for(int i=0; i<numAmount; i++){
				numForAnalize.set_num(rand()%1001);
				std::cout<<numForAnalize.num()<<" ";
			
				stream->Write(numForAnalize);		//send number to server				
			}
			std::cout<<std::endl;
			stream->WritesDone();
		});
		
		//Get responce
		NumberInfo numInfo;
		while (stream->Read(&numInfo)){
			//print number in column
			std::cout<<"Number in column: "<<std::endl;
			for(int i=0; i<numInfo.numamount();i++){
				std::cout<<numInfo.numarr(i)<<std::endl;
			}
			
			//other info
			std::cout<<"Amount of numerals: "<<numInfo.numamount()<<std::endl;
			if(numInfo.even()){
				std::cout<<"Number is even"<<std::endl;
			}
			else{
				std::cout<<"Number is odd"<<std::endl;
			}			
		}
		
		writer.join();	//wait for end of thread
		
		Status status = stream->Finish();
		if (!status.ok()) {
			std::cout << "rpc failed." << std::endl;
		}
	}
	
	void Chat(){
		Text strToSend; 
		Text strGet; 		
        
		while(true){
			std::string str;
			std::cout<<"You (\"~\"for exit): ";
			std::cin.ignore();
			getline(std::cin, str);
        
			if(str=="~"){
				return;
			}
		
			strToSend.set_text(str);
		
			ClientContext context;
			Status status = stub_->Chat(&context, strToSend, &strGet);
		
			if (!status.ok()) {
				std::cout << "Error..." << std::endl;            
			}
			
			std::cout << "Someone: "<< strGet.text() <<  std::endl; 
			}		
	}
	

private:
    bool MakeOneNumeralConversation(const Numeral& num, TextNumeral* textNum){
        ClientContext context;  //context object for rpc

        //Call rpc by name from stub
        Status status = stub_->NumeralConverter(&context, num, textNum);

        //if return status isn't ok
        if (!status.ok()) {
            std::cout << "NumeralConverter rpc failed" << std::endl;
            return false;
        }

        //check if some data return
        if (textNum->textnum().empty()){
            std::cout << "Server returns incomplete data" << std::endl;
            return false;
        }

        //Print information from server
        std::cout << "Converted value for "<< num.num() << " is: "<< textNum->textnum() << std::endl;

        return true;
    }

    //Stub for client
    std::unique_ptr<InfoSearcher::Stub> stub_;
};

//Example function
void NumberConverter(InfoSearcherClient& client){
	int num;
    while(true){
        std::cout<<"Input 0-9 for convert, or bigger for exit: ";
        std::cin>>num;
        if (num<0 || num>9) break;

        client.NumeralConverter(num);
    }
}

void SimpleNumber(InfoSearcherClient& client){
	int num;
    while(true){
        std::cout<<"Input limit for simple number, or 0 for exit: ";
        std::cin>>num;
		
        if (num==0) break;
        client.SimpleNumbersInRange(num);
    }
}

void SimpleAmount(InfoSearcherClient& client){
	int num;
    while(true){
        std::cout<<"Input amount of random number, or 0 for exit: ";
        std::cin>>num;
		
        if (num==0) break;
        client.CalculateSimpleNumber(num);
    }
}

void NumberInfo(InfoSearcherClient& client){
	int num;
    while(true){
        std::cout<<"Input amount of random number for analisis, or 0 for exit: ";
        std::cin>>num;
		
        if (num==0) break;
        client.RandomNumberAnalize(num);
    }
}

int main(int argc, char** argv){
	std::string ip;
	std::string port;
	std::cout<<"Enter IP: ";
	std::cin>>ip;
	std::cout<<"Enter port: ";
	std::cin>>port;
	std::string server_address = ip+":"+port;
    //std::string server_address("172.18.147.199:25565");
    //Instance of class worked with service
    InfoSearcherClient client(grpc::CreateChannel(server_address,grpc::InsecureChannelCredentials()));

	while(true){
		int choose;
		std::cout<<"1 - Number convert to text (Unary rpc)" <<std::endl;
		std::cout<<"2 - Simple number from 1 to N (Server stream rpc)" <<std::endl;
		std::cout<<"3 - Amount of simple in random number (Client stream rpc)" <<std::endl;
		std::cout<<"4 - Analize number (Client and server stream rpc)" <<std::endl;
		std::cout<<"5 - Chat" <<std::endl;
		std::cout<<"other - Exit" <<std::endl;
		std::cout<<"Choose: " <<std::endl;
		std::cin>>choose;
		
		if(choose==1){
			NumberConverter(client);
		}
		else if(choose==2){
			SimpleNumber(client);
		}
		else if(choose==3){
			SimpleAmount(client);
		}	
		else if(choose==4){
			NumberInfo(client);
		}	
		else if(choose==5){
			client.Chat();
		}	
		else{
			break;
		}		
		
	}

    
}