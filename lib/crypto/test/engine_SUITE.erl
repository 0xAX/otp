%%
%% %CopyrightBegin%
%%
%% Copyright Ericsson AB 2017-2017. All Rights Reserved.
%%
%% Licensed under the Apache License, Version 2.0 (the "License");
%% you may not use this file except in compliance with the License.
%% You may obtain a copy of the License at
%%
%%     http://www.apache.org/licenses/LICENSE-2.0
%%
%% Unless required by applicable law or agreed to in writing, software
%% distributed under the License is distributed on an "AS IS" BASIS,
%% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
%% See the License for the specific language governing permissions and
%% limitations under the License.
%%
%% %CopyrightEnd%
%%
%%

-module(engine_SUITE).

-include_lib("common_test/include/ct.hrl").

%% Note: This directive should only be used in test suites.
-compile(export_all).

%%--------------------------------------------------------------------
%% Common Test interface functions -----------------------------------
%%--------------------------------------------------------------------
suite() ->
    [{ct_hooks,[ts_install_cth]},
     {timetrap,{seconds, 10}}
    ].

all() ->
    [
     get_all_possible_methods,
     engine_load_all_methods,
     engine_load_some_methods,
     bad_arguments,
     unknown_engine,
     pre_command_fail_bad_value,
     pre_command_fail_bad_key,
     failed_engine_init
    ].

init_per_suite(Config) ->    
    try crypto:start() of
	ok ->
            Config;
        {error,{already_started,crypto}} ->
            Config
    catch _:_ ->
	    {skip, "Crypto did not start"}
    end.
end_per_suite(_Config) ->
    ok.

%%--------------------------------------------------------------------
init_per_group(_Group, Config0) ->
    Config0.

end_per_group(_, _) ->
    ok.

%%--------------------------------------------------------------------
init_per_testcase(_Case, Config) ->
    Config.
end_per_testcase(_Case, _Config) ->
    ok.

%%-------------------------------------------------------------------------
%% Test cases starts here.
%%-------------------------------------------------------------------------
get_all_possible_methods() ->
    [{doc, "Just fetch all possible engine methods supported."}].

get_all_possible_methods(Config) when is_list(Config) ->
    try
        List = crypto:engine_get_all_methods(),
        ct:log("crypto:engine_get_all_methods() -> ~p\n", [List]),
        ok
    catch
        error:notsup ->
            {skip, "Engine not supported on this OpenSSL version"}
    end.

engine_load_all_methods()->
    [{doc, "Use a dummy md5 engine that does not implement md5"
      "but rather returns a static binary to test that crypto:engine_load "
      "functions works."}].

engine_load_all_methods(Config) when is_list(Config) ->
    case crypto:get_test_engine() of
        {error, notexist} ->
            {skip, "OTP Test engine not found"};
        {ok, Engine} ->
            try 
                Md5Hash1 =  <<106,30,3,246,166,222,229,158,244,217,241,179,50,232,107,109>>,
                Md5Hash1 = crypto:hash(md5, "Don't panic"),
                Md5Hash2 =  <<0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15>>,
                case crypto:engine_load(<<"dynamic">>,
                                        [{<<"SO_PATH">>, Engine},
                                         {<<"ID">>, <<"MD5">>},
                                         <<"LOAD">>],
                                        []) of
                    {ok, E} ->
                        case crypto:hash(md5, "Don't panic") of
                            Md5Hash1 ->
                                ct:fail(fail_to_load_still_original_engine);
                            Md5Hash2 ->
                                ok;
                            _ ->
                                ct:fail(fail_to_load_engine)
                        end,
                        ok = crypto:engine_unload(E),
                        case crypto:hash(md5, "Don't panic") of
                            Md5Hash2 ->
                                ct:fail(fail_to_unload_still_test_engine);
                            Md5Hash1 ->
                                ok;
                            _ ->
                                ct:fail(fail_to_unload_engine)
                        end;
                    {error, bad_engine_id} ->
                        {skip, "Dynamic Engine not supported"}
                end
           catch
               error:notsup ->
                  {skip, "Engine not supported on this OpenSSL version"}
           end
    end.

engine_load_some_methods()->
    [{doc, "Use a dummy md5 engine that does not implement md5"
      "but rather returns a static binary to test that crypto:engine_load "
      "functions works."}].

engine_load_some_methods(Config) when is_list(Config) ->
    case crypto:get_test_engine() of
        {error, notexist} ->
            {skip, "OTP Test engine not found"};
        {ok, Engine} ->
            try 
                Md5Hash1 =  <<106,30,3,246,166,222,229,158,244,217,241,179,50,232,107,109>>,
                Md5Hash1 = crypto:hash(md5, "Don't panic"),
                Md5Hash2 =  <<0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15>>,
                EngineMethods = crypto:engine_get_all_methods() --
                    [engine_method_dh,engine_method_rand,
                     engine_method_ciphers, engine_method_store,
                     engine_method_pkey_meths, engine_method_pkey_asn1_meths],
                case crypto:engine_load(<<"dynamic">>,
                                        [{<<"SO_PATH">>, Engine},
                                         {<<"ID">>, <<"MD5">>},
                                         <<"LOAD">>],
                                        [],
                                        EngineMethods) of
                    {ok, E} ->        
                        case crypto:hash(md5, "Don't panic") of
                            Md5Hash1 ->
                                ct:fail(fail_to_load_engine_still_original);
                            Md5Hash2 ->
                                ok;
                            _ ->
                                ct:fail(fail_to_load_engine)
                        end,
                        ok = crypto:engine_unload(E),
                        case crypto:hash(md5, "Don't panic") of
                            Md5Hash2 ->
                                ct:fail(fail_to_unload_still_test_engine);
                            Md5Hash1 ->
                                ok;
                            _ ->
                                ct:fail(fail_to_unload_engine)
                        end;
                    {error, bad_engine_id} ->
                    {skip, "Dynamic Engine not supported"}
                end
           catch
               error:notsup ->
                  {skip, "Engine not supported on this OpenSSL version"}
           end
    end.

%%-------------------------------------------------------------------------
%% Error cases
bad_arguments()->
    [{doc, "Test different arguments in bad format."}].

bad_arguments(Config) when is_list(Config) ->
    case crypto:get_test_engine() of
        {error, notexist} ->
            {skip, "OTP Test engine not found"};
        {ok, Engine} ->
            try 
                try
                    crypto:engine_load(fail_engine, [], [])
                catch
                    error:badarg ->
                       ok
                end,
                try
                    crypto:engine_load(<<"dynamic">>,
                                       [{<<"SO_PATH">>, Engine},
                                        1,
                                        {<<"ID">>, <<"MD5">>},
                                        <<"LOAD">>],
                                       [])
                catch
                    error:badarg ->
                       ok
                end,
                try
                    crypto:engine_load(<<"dynamic">>,
                                       [{<<"SO_PATH">>, Engine},
                                        {'ID', <<"MD5">>},
                                        <<"LOAD">>],
                                       [])
                catch
                    error:badarg ->
                       ok
                end
          catch
              error:notsup ->
                 {skip, "Engine not supported on this OpenSSL version"}
          end
    end.

unknown_engine() ->
    [{doc, "Try to load a non existent engine."}].

unknown_engine(Config) when is_list(Config) ->
    try
        {error, bad_engine_id} = crypto:engine_load(<<"fail_engine">>, [], []),
        ok
    catch
        error:notsup ->
           {skip, "Engine not supported on this OpenSSL version"}
    end.

pre_command_fail_bad_value() ->
    [{doc, "Test pre command due to bad value"}].

pre_command_fail_bad_value(Config) when is_list(Config) ->
    DataDir = unicode:characters_to_binary(code:priv_dir(crypto)),
    try
        case crypto:engine_load(<<"dynamic">>,
                                [{<<"SO_PATH">>,
                                  <<DataDir/binary, <<"/libfail_engine.so">>/binary >>},
                                 {<<"ID">>, <<"MD5">>},
                                 <<"LOAD">>],
                                []) of
            {error, ctrl_cmd_failed} ->
                ok;
            {error, bad_engine_id} ->
                {skip, "Dynamic Engine not supported"}
        end
    catch
        error:notsup ->
           {skip, "Engine not supported on this OpenSSL version"}
    end.

pre_command_fail_bad_key() ->
    [{doc, "Test pre command due to bad key"}].

pre_command_fail_bad_key(Config) when is_list(Config) ->
    try
        case crypto:get_test_engine() of
            {error, notexist} ->
                {skip, "OTP Test engine not found"};
            {ok, Engine} ->
                case crypto:engine_load(<<"dynamic">>,
                                        [{<<"SO_WRONG_PATH">>, Engine},
                                         {<<"ID">>, <<"MD5">>},
                                         <<"LOAD">>],
                                        []) of
                    {error, ctrl_cmd_failed} ->
                        ok;
                    {error, bad_engine_id} ->
                        {skip, "Dynamic Engine not supported"}
                end
        end
   catch 
       error:notsup ->
          {skip, "Engine not supported on this OpenSSL version"}
   end.

failed_engine_init()->
    [{doc, "Test failing engine init due to missed pre command"}].

failed_engine_init(Config) when is_list(Config) ->
    try
        case crypto:get_test_engine() of
            {error, notexist} ->
                {skip, "OTP Test engine not found"};
            {ok, Engine} ->
                case crypto:engine_load(<<"dynamic">>,
                                        [{<<"SO_PATH">>, Engine},
                                         {<<"ID">>, <<"MD5">>}],
                                        []) of
                    {error, add_engine_failed} ->
                        ok;
                    {error, bad_engine_id} ->
                        {skip, "Dynamic Engine not supported"}
                end
        end
   catch 
       error:notsup ->
          {skip, "Engine not supported on this OpenSSL version"}
   end.
