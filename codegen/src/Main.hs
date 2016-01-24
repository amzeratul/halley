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

module Main where

import System.Environment
import Data.List
import Control.Monad

import Halley.Parser
import Halley.AST
import Halley.SemanticAnalysis
import Halley.CodeGenCpp

import Text.ParserCombinators.Parsec


---------------------------


main = do
    args <- getArgs
    parseStage args

parseStage args = do
    parseResult <- parseFiles args
    case parseResult of
        Left error -> putStrLn $ show error
        Right defs -> semanticStage defs

semanticStage :: [GenDefinition] -> IO ()
semanticStage defs = do
    case semanticAnalysis defs of
        Left error -> putStrLn error
        Right dataToGen -> codeGenStage dataToGen

codeGenStage :: CodeGenData -> IO ()
codeGenStage dataToGen = do
    printStage $ generateCodeCpp dataToGen

printStage :: [String] -> IO ()
printStage defs = mapM_ (\x -> putStrLn $ '\n' : x) defs

parseFiles fs = do
    fmap (foldEither) (mapM (parse) fs)
    where
        parse f = fmap (parseFile f) (readFile f)

foldEither vs = foldl' (\a b -> (++) <$> a <*> b) (Right []) vs
