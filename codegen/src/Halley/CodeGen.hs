{-
   Copyright 2016 Rodrigo Braz Monteiro

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
-}
module Halley.CodeGen
( GeneratedSource(GeneratedSource, filename, code)
) where

data GeneratedSource = GeneratedSource { filename :: String
                                       , code :: String
                                       }

instance Show GeneratedSource where
    show a = filename a ++ "\n" ++ (code a)
