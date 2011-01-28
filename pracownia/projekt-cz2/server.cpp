//
// blocking_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>

#include "common.hpp"
#include "utils.hpp"

using boost::asio::ip::tcp;

const int max_length = 1024;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

View * currentView;

std::string parseLine(std::string data)
{
  std::stringstream ss;
  boost::trim(data);
  std::vector<std::string> strs;
  boost::split(strs, data, boost::is_any_of("\t \n"));
  if (strs.size() < 1)
    return "==???\n";

  try
    {
      if( boost::starts_with( std::string("quit"), strs[0] ) )
        {
          return "QUIT";
        }

      if( boost::starts_with( std::string("help"), strs[0] ) )
        {
          std::string sep = "\n\t"; 
          ss << "COMMANDS: " << sep
             << "quit" << sep
             << ("dirvec") << sep
             << ("start") << sep 
             << ("steps") << sep 
             << ("surf") << sep 
             << ("range_w") << sep
             << ("rw") << sep
             << ("range_h") << sep
             << ("rh") << sep
             << ("rr") << sep
             << std::endl;

          return ss.str();
        }


      if( boost::starts_with( std::string("dirvec"), strs[0] ) )
        {
          if (strs.size() < 1+3) return "ARG???\n";
      
          float x, y, z;
          x = boost::lexical_cast< float >( strs[1] );
          y = boost::lexical_cast< float >( strs[2] );
          z = boost::lexical_cast< float >( strs[3] );
          currentView->DirectionVector = make_float3( x, y, z );
        }

      if( boost::starts_with( std::string("start"), strs[0] ) )
        {
          if (strs.size() < 1+3) return "ARG???\n";

          float x, y, z;
          x = boost::lexical_cast< float >( strs[1] );
          y = boost::lexical_cast< float >( strs[2] );
          z = boost::lexical_cast< float >( strs[3] );
          currentView->StartingPoint = make_float3( x, y, z );
        }

      if( boost::starts_with( std::string("start.x"), strs[0] ) )
        {
          if (strs.size() < 1+1) return "ARG???\n";
          currentView->StartingPoint.x = boost::lexical_cast< float >( strs[1] );;
        }

      if( boost::starts_with( std::string("start.y"), strs[0] ) )
        {
          if (strs.size() < 1+1) return "ARG???\n";
          currentView->StartingPoint.y = boost::lexical_cast< float >( strs[1] );;
        }

      if( boost::starts_with( std::string("start.z"), strs[0] ) )
        {
          if (strs.size() < 1+1) return "ARG???\n";
          currentView->StartingPoint.z = boost::lexical_cast< float >( strs[1] );;
        }



      if( boost::starts_with( std::string("dirvec.x"), strs[0] ) )
        {
          if (strs.size() < 1+1) return "ARG???\n";
          currentView->DirectionVector.x = boost::lexical_cast< float >( strs[1] );;
        }

      if( boost::starts_with( std::string("dirvec.y"), strs[0] ) )
        {
          if (strs.size() < 1+1) return "ARG???\n";
          currentView->DirectionVector.y = boost::lexical_cast< float >( strs[1] );;
        }

      if( boost::starts_with( std::string("dirvec.z"), strs[0] ) )
        {
          if (strs.size() < 1+1) return "ARG???\n";
          currentView->DirectionVector.z = boost::lexical_cast< float >( strs[1] );;
        }

      if( boost::starts_with( std::string("steps"), strs[0] ) )
        {
          if (strs.size() < 1+1) return "ARG???\n";

          int x;
          x = (int)boost::lexical_cast< float >( strs[1] );
          currentView->steps = x;
        }

      if( boost::starts_with( std::string("surf"), strs[0] ) )
        {
          if (strs.size() < 1+1) return "ARG???\n";
          currentView->surf = (Surf)(int)boost::lexical_cast< float >( strs[1] );
        }

      if( ( boost::starts_with( std::string("range_w"), strs[0] ) )
          || ( boost::starts_with( std::string("rw"), strs[0] ) )) 
        {
          if (strs.size() < 1+1) return "ARG???\n";

          float x;
          x = boost::lexical_cast< float >( strs[1] );
          currentView->range_w = x;
        }

      if( ( boost::starts_with( std::string("range_h"), strs[0] ) )
          || ( boost::starts_with( std::string("rh"), strs[0] ) )) 
        {
          if (strs.size() < 1+1) return "ARG???\n";

          float x;
          x = boost::lexical_cast< float >( strs[1] );
          currentView->range_h = x;
        }

      if( ( boost::starts_with( std::string("rr"), strs[0] ) ) )
        {
          if (strs.size() < 1+1) return "ARG???\n";

          float x;
          x = boost::lexical_cast< float >( strs[1] );
          currentView->range_h = x;
          currentView->range_w = x;
        }
    }
  catch ( std::exception & e )
    {
      ss << "EXCEPTION: " << e.what() << std::endl;
    }  

  PrintView( *currentView, ss );

  return ss.str();
}

void session(socket_ptr sock)
{
  try
  {
    for (;;)
    {
      char data[max_length];
      memset((void*)data, '\0', max_length);

      boost::system::error_code error;
      size_t length = sock->read_some(boost::asio::buffer(data), error);
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.
      
      // echo
      boost::asio::write(*sock, boost::asio::buffer(data, length));

      // parse line
      std::string resp = parseLine( std::string(data) );
      std::stringstream ss(resp);
      boost::asio::write(*sock, boost::asio::buffer(ss.str().c_str(), ss.str().length()));

      if (resp == "QUIT")
        break;

    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception in thread: " << e.what() << "\n";
  }
}

void server(boost::asio::io_service& io_service, short port)
{
  tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
  for (;;)
  {
    socket_ptr sock(new tcp::socket(io_service));
    a.accept(*sock);
    boost::thread t(boost::bind(session, sock));
  }
}

void * server_thread(void * arg)
{
  currentView = (View*) arg;

  try
  {
    boost::asio::io_service io_service;
    server(io_service, SERV_PORT);
  }
  catch (std::exception& e)
    {
      std::cerr << "Exception: " << e.what() << "\n";
    }

  return NULL;
}