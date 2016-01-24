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
import qualified Halley.SemanticAnalysis as Semantics
import qualified Halley.CodeGenCpp as CodeGen

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
    case Semantics.semanticAnalysis defs of
        Left error -> putStrLn error
        Right dataToGen -> do
            putStrLn("----------\nComponents:\n")
            mapM_ (\x -> putStrLn $ show x ++ "\n") $ Semantics.components dataToGen
            putStrLn("----------\nSystems:\n")
            mapM_ (\x -> putStrLn $ show x ++ "\n") $ Semantics.systems dataToGen
            codeGenStage dataToGen

codeGenStage :: Semantics.CodeGenData -> IO ()
codeGenStage dataToGen = do
    putStrLn("----------\nData generated:\n")
    mapM_ (\x -> putStrLn (x ++ "\n")) (CodeGen.generateCodeCpp dataToGen)

parseFiles fs = do
    fmap (foldEither) (mapM (parse) fs)
    where
        parse f = fmap (parseFile f) (readFile f)

foldEither vs = foldl' (\a b -> (++) <$> a <*> b) (Right []) vs
